#pragma once

#include "RenderResource.h"
#include <vector>

namespace happycat {

class RenderGraphBuilder;

// Resource builder - used in DeclareResources()
class ResourceBuilder {
public:
    explicit ResourceBuilder(RenderGraphBuilder* graph) : m_Graph(graph) {}

    // Create texture resource
    TextureHandle CreateTexture(const GraphTextureDesc& desc);

    // Create buffer resource
    BufferHandle CreateBuffer(const GraphBufferDesc& desc);

    // Import external texture
    TextureHandle ImportTexture(void* externalTexture);

    // Import external buffer
    BufferHandle ImportBuffer(void* externalBuffer);

private:
    RenderGraphBuilder* m_Graph;
};

// Dependency builder - used in DeclareDependencies()
class DependencyBuilder {
public:
    DependencyBuilder(RenderGraphBuilder* graph, RenderPass* pass)
        : m_Graph(graph), m_Pass(pass) {}

    // Read resource
    void Read(TextureHandle texture);
    void Read(BufferHandle buffer);

    // Write resource
    void Write(TextureHandle texture);
    void Write(BufferHandle buffer);

    // Read/Write resource
    void ReadWrite(TextureHandle texture);
    void ReadWrite(BufferHandle buffer);

    // Explicit dependency on another pass
    void DependsOn(const std::string& passName);

private:
    RenderGraphBuilder* m_Graph;
    RenderPass* m_Pass;
};

} // namespace happycat
