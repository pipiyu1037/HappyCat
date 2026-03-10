#include "RenderGraphBuilder.h"
#include "ResourceBuilder.h"
#include "DependencyBuilder.h"
#include "RHI/Vulkan/VKDevice.h"
#include "RHI/Vulkan/VKCommandBuffer.h"
#include "Core/Utils/Logger.h"
#include "Engine/FrameContext.h"
#include <queue>
#include <algorithm>

namespace happycat {

// ResourceBuilder implementation
TextureHandle ResourceBuilder::CreateTexture(const GraphTextureDesc& desc) {
    return m_Graph->CreateTexture(desc);
}

BufferHandle ResourceBuilder::CreateBuffer(const GraphBufferDesc& desc) {
    return m_Graph->CreateBuffer(desc);
}

TextureHandle ResourceBuilder::ImportTexture(void* externalTexture) {
    // TODO: Import external texture
    return TextureHandle{};
}

BufferHandle ResourceBuilder::ImportBuffer(void* externalBuffer) {
    // TODO: Import external buffer
    return BufferHandle{};
}

// DependencyBuilder implementation
void DependencyBuilder::Read(TextureHandle texture) {
    m_Graph->AddTextureRead(m_PassIndex, texture);
}

void DependencyBuilder::Read(BufferHandle buffer) {
    m_Graph->AddBufferRead(m_PassIndex, buffer);
}

void DependencyBuilder::Write(TextureHandle texture) {
    m_Graph->AddTextureWrite(m_PassIndex, texture);
}

void DependencyBuilder::Write(BufferHandle buffer) {
    m_Graph->AddBufferWrite(m_PassIndex, buffer);
}

void DependencyBuilder::ReadWrite(TextureHandle texture) {
    Read(texture);
    Write(texture);
}

void DependencyBuilder::ReadWrite(BufferHandle buffer) {
    Read(buffer);
    Write(buffer);
}

void DependencyBuilder::DependsOn(const std::string& passName) {
    m_Graph->AddPassDependency(m_PassIndex, passName);
}

// RenderGraphBuilder implementation
RenderGraphBuilder::RenderGraphBuilder(VKDevice* device)
    : m_Device(device)
{
}

RenderGraphBuilder::~RenderGraphBuilder() {
}

TextureHandle RenderGraphBuilder::CreateTexture(const GraphTextureDesc& desc) {
    TextureResource resource;
    resource.handle.handle = m_NextHandle++;
    resource.name = desc.name;
    resource.desc = desc;
    resource.transient = true;

    m_Textures.push_back(resource);
    return resource.handle;
}

BufferHandle RenderGraphBuilder::CreateBuffer(const GraphBufferDesc& desc) {
    BufferResource resource;
    resource.handle.handle = m_NextHandle++;
    resource.name = desc.name;
    resource.desc = desc;
    resource.transient = true;

    m_Buffers.push_back(resource);
    return resource.handle;
}

void RenderGraphBuilder::AddTextureRead(u32 passIndex, TextureHandle texture) {
    if (passIndex < m_PassNodes.size()) {
        PassDependency dep;
        dep.texture = texture;
        dep.isTexture = true;
        dep.isWrite = false;
        m_PassNodes[passIndex].dependencies.push_back(dep);
    }
}

void RenderGraphBuilder::AddTextureWrite(u32 passIndex, TextureHandle texture) {
    if (passIndex < m_PassNodes.size()) {
        PassDependency dep;
        dep.texture = texture;
        dep.isTexture = true;
        dep.isWrite = true;
        m_PassNodes[passIndex].dependencies.push_back(dep);
    }
}

void RenderGraphBuilder::AddBufferRead(u32 passIndex, BufferHandle buffer) {
    if (passIndex < m_PassNodes.size()) {
        PassDependency dep;
        dep.buffer = buffer;
        dep.isTexture = false;
        dep.isWrite = false;
        m_PassNodes[passIndex].dependencies.push_back(dep);
    }
}

void RenderGraphBuilder::AddBufferWrite(u32 passIndex, BufferHandle buffer) {
    if (passIndex < m_PassNodes.size()) {
        PassDependency dep;
        dep.buffer = buffer;
        dep.isTexture = false;
        dep.isWrite = true;
        m_PassNodes[passIndex].dependencies.push_back(dep);
    }
}

void RenderGraphBuilder::AddPassDependency(u32 passIndex, const std::string& passName) {
    auto it = m_PassNameToIndex.find(passName);
    if (it != m_PassNameToIndex.end()) {
        u32 depPassIndex = it->second;
        if (passIndex < m_PassNodes.size() && depPassIndex < m_PassNodes.size()) {
            m_PassNodes[passIndex].dependsOnPasses.push_back(depPassIndex);
            m_PassNodes[depPassIndex].dependentPasses.push_back(passIndex);
        }
    } else {
        HC_CORE_WARN("Pass '{0}' not found for dependency", passName);
    }
}

void RenderGraphBuilder::BuildDependencyGraph() {
    // Build implicit dependencies based on resource access
    // Map: resource handle -> last pass that wrote to it
    std::unordered_map<u64, u32> lastWriterTexture;
    std::unordered_map<u64, u32> lastWriterBuffer;

    for (auto& node : m_PassNodes) {
        for (const auto& dep : node.dependencies) {
            if (dep.isTexture) {
                u64 handle = dep.texture.handle;
                if (dep.isWrite) {
                    // If another pass wrote to this texture before, we depend on it
                    auto it = lastWriterTexture.find(handle);
                    if (it != lastWriterTexture.end() && it->second != node.index) {
                        // Add implicit dependency
                        bool alreadyDepends = false;
                        for (u32 depIdx : node.dependsOnPasses) {
                            if (depIdx == it->second) {
                                alreadyDepends = true;
                                break;
                            }
                        }
                        if (!alreadyDepends) {
                            node.dependsOnPasses.push_back(it->second);
                            m_PassNodes[it->second].dependentPasses.push_back(node.index);
                        }
                    }
                    lastWriterTexture[handle] = node.index;
                } else {
                    // Read: depend on last writer
                    auto it = lastWriterTexture.find(handle);
                    if (it != lastWriterTexture.end() && it->second != node.index) {
                        bool alreadyDepends = false;
                        for (u32 depIdx : node.dependsOnPasses) {
                            if (depIdx == it->second) {
                                alreadyDepends = true;
                                break;
                            }
                        }
                        if (!alreadyDepends) {
                            node.dependsOnPasses.push_back(it->second);
                            m_PassNodes[it->second].dependentPasses.push_back(node.index);
                        }
                    }
                }
            } else {
                // Buffer
                u64 handle = dep.buffer.handle;
                if (dep.isWrite) {
                    auto it = lastWriterBuffer.find(handle);
                    if (it != lastWriterBuffer.end() && it->second != node.index) {
                        bool alreadyDepends = false;
                        for (u32 depIdx : node.dependsOnPasses) {
                            if (depIdx == it->second) {
                                alreadyDepends = true;
                                break;
                            }
                        }
                        if (!alreadyDepends) {
                            node.dependsOnPasses.push_back(it->second);
                            m_PassNodes[it->second].dependentPasses.push_back(node.index);
                        }
                    }
                    lastWriterBuffer[handle] = node.index;
                } else {
                    auto it = lastWriterBuffer.find(handle);
                    if (it != lastWriterBuffer.end() && it->second != node.index) {
                        bool alreadyDepends = false;
                        for (u32 depIdx : node.dependsOnPasses) {
                            if (depIdx == it->second) {
                                alreadyDepends = true;
                                break;
                            }
                        }
                        if (!alreadyDepends) {
                            node.dependsOnPasses.push_back(it->second);
                            m_PassNodes[it->second].dependentPasses.push_back(node.index);
                        }
                    }
                }
            }
        }
    }
}

bool RenderGraphBuilder::TopologicalSort() {
    // Kahn's algorithm for topological sort
    std::vector<u32> inDegree(m_PassNodes.size(), 0);

    // Calculate in-degree for each node
    for (const auto& node : m_PassNodes) {
        inDegree[node.index] = static_cast<u32>(node.dependsOnPasses.size());
    }

    // Queue of nodes with no dependencies
    std::queue<u32> queue;
    for (size_t i = 0; i < m_PassNodes.size(); ++i) {
        if (inDegree[i] == 0) {
            queue.push(static_cast<u32>(i));
        }
    }

    // Process queue
    std::vector<u32> sortedOrder;
    while (!queue.empty()) {
        u32 nodeIdx = queue.front();
        queue.pop();
        sortedOrder.push_back(nodeIdx);

        // Reduce in-degree of dependent nodes
        for (u32 depIdx : m_PassNodes[nodeIdx].dependentPasses) {
            inDegree[depIdx]--;
            if (inDegree[depIdx] == 0) {
                queue.push(depIdx);
            }
        }
    }

    // Check for cycles
    if (sortedOrder.size() != m_PassNodes.size()) {
        HC_CORE_ERROR("Cyclic dependency detected in render graph!");
        return false;
    }

    // Update sorted indices
    for (size_t i = 0; i < sortedOrder.size(); ++i) {
        m_PassNodes[sortedOrder[i]].sortedIndex = static_cast<u32>(i);
    }

    // Reorder passes based on sorted order
    std::vector<std::unique_ptr<RenderPass>> sortedPasses;
    sortedPasses.reserve(m_Passes.size());
    for (u32 idx : sortedOrder) {
        sortedPasses.push_back(std::move(m_Passes[m_PassNodes[idx].index]));
    }
    m_Passes = std::move(sortedPasses);

    // Update pass pointers in nodes
    for (size_t i = 0; i < m_Passes.size(); ++i) {
        m_PassNodes[sortedOrder[i]].pass = m_Passes[i].get();
        m_PassNodes[sortedOrder[i]].sortedIndex = static_cast<u32>(i);
    }

    return true;
}

void RenderGraphBuilder::Compile() {
    if (m_Compiled) return;

    HC_CORE_INFO("Compiling render graph with {0} passes...", m_Passes.size());

    // 1. Build dependency graph from resource access patterns
    BuildDependencyGraph();

    // 2. Topological sort
    if (!TopologicalSort()) {
        HC_CORE_ERROR("Render graph compilation failed due to cyclic dependencies");
        return;
    }

    m_Compiled = true;
    HC_CORE_INFO("Render graph compiled successfully");
}

void RenderGraphBuilder::Execute(FrameContext& ctx, u32 width, u32 height) {
    if (!m_Compiled) {
        Compile();
    }

    RenderPassContext passCtx{};
    passCtx.width = width;
    passCtx.height = height;
    passCtx.frameIndex = ctx.GetCurrentFrameIndex();

    // Get command buffer
    VKCommandBuffer* cmd = ctx.GetCurrentCommandBuffer();

    // Execute passes in sorted order
    for (auto& pass : m_Passes) {
        if (pass->ShouldExecute()) {
            pass->Execute(passCtx, *cmd);
        }
    }
}

void RenderGraphBuilder::Reset() {
    m_Passes.clear();
    m_PassNodes.clear();
    m_PassNameToIndex.clear();
    m_Textures.clear();
    m_Buffers.clear();
    m_Compiled = false;
    m_NextHandle = 1;
}

} // namespace happycat
