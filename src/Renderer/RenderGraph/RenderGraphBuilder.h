#pragma once

#include "RenderPass.h"
#include "RenderResource.h"
#include "ResourceBuilder.h"
#include "DependencyBuilder.h"
#include "Core/Utils/Types.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace happycat {

class VKDevice;
class VKCommandBuffer;
class FrameContext;

// Resource dependency tracking
struct PassDependency {
    TextureHandle texture;
    BufferHandle buffer;
    bool isTexture;  // true=texture, false=buffer
    bool isWrite;    // true=write, false=read
};

// Pass node for dependency graph
struct PassNode {
    RenderPass* pass = nullptr;
    std::string name;
    u32 index = 0;
    std::vector<PassDependency> dependencies;
    std::vector<u32> dependsOnPasses;    // Passes this depends on
    std::vector<u32> dependentPasses;     // Passes that depend on this
    bool visited = false;
    u32 sortedIndex = 0;
};

// Render graph builder
class RenderGraphBuilder {
public:
    explicit RenderGraphBuilder(VKDevice* device);
    ~RenderGraphBuilder();

    // Add pass
    template<typename PassType, typename... Args>
    PassType* AddPass(Args&&... args) {
        auto pass = std::make_unique<PassType>(std::forward<Args>(args)...);
        PassType* ptr = pass.get();

        // Create pass node
        PassNode node;
        node.pass = pass.get();
        node.name = pass->GetName();
        node.index = static_cast<u32>(m_Passes.size());

        // Declare resources
        ResourceBuilder resourceBuilder(this);
        pass->DeclareResources(resourceBuilder);

        // Declare dependencies
        DependencyBuilder depBuilder(this, pass.get(), node.index);
        pass->DeclareDependencies(depBuilder);

        m_PassNodes.push_back(std::move(node));
        m_PassNameToIndex[node.name] = node.index;
        m_Passes.push_back(std::move(pass));
        return *ptr;
    }

    // Compile graph
    void Compile();

    // Execute graph
    void Execute(FrameContext& ctx, u32 width, u32 height);

    // Reset for next frame
    void Reset();

    // Resource creation (called by ResourceBuilder)
    TextureHandle CreateTexture(const GraphTextureDesc& desc);
    BufferHandle CreateBuffer(const GraphBufferDesc& desc);

    // Dependency tracking (called by DependencyBuilder)
    void AddTextureRead(u32 passIndex, TextureHandle texture);
    void AddTextureWrite(u32 passIndex, TextureHandle texture);
    void AddBufferRead(u32 passIndex, BufferHandle buffer);
    void AddBufferWrite(u32 passIndex, BufferHandle buffer);
    void AddPassDependency(u32 passIndex, const std::string& passName);

    // Set backbuffer
    void SetBackbuffer(TextureHandle backbuffer) { m_Backbuffer = backbuffer; }
    TextureHandle GetBackbuffer() const { return m_Backbuffer; }

private:
    // Compilation helpers
    void BuildDependencyGraph();
    bool TopologicalSort();

    VKDevice* m_Device;
    std::vector<std::unique_ptr<RenderPass>> m_Passes;

    // Pass nodes for dependency tracking
    std::vector<PassNode> m_PassNodes;
    std::unordered_map<std::string, u32> m_PassNameToIndex;

    // Resources
    std::vector<TextureResource> m_Textures;
    std::vector<BufferResource> m_Buffers;

    TextureHandle m_Backbuffer;
    ResourceHandle m_NextHandle = 1;

    bool m_Compiled = false;
};

} // namespace happycat
