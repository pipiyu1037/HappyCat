#pragma once

#include "RenderPass.h"
#include "RenderResource.h"
#include "Core/Utils/Types.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace happycat {

class VKDevice;
class VKCommandBuffer;
class FrameContext;

// Render graph builder
class RenderGraphBuilder {
public:
    explicit RenderGraphBuilder(VKDevice* device);
    ~RenderGraphBuilder();

    // Add pass
    template<typename PassType, typename... Args>
    PassType& AddPass(Args&&... args) {
        auto pass = std::make_unique<PassType>(std::forward<Args>(args)...);
        PassType* ptr = pass.get();

        // Declare resources
        ResourceBuilder resourceBuilder(this);
        pass->DeclareResources(resourceBuilder);

        // Declare dependencies
        DependencyBuilder depBuilder(this, pass.get());
        pass->DeclareDependencies(depBuilder);

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

    // Set backbuffer
    void SetBackbuffer(TextureHandle backbuffer) { m_Backbuffer = backbuffer; }
    TextureHandle GetBackbuffer() const { return m_Backbuffer; }

private:
    VKDevice* m_Device;
    std::vector<std::unique_ptr<RenderPass>> m_Passes;

    // Resources
    std::vector<TextureResource> m_Textures;
    std::vector<BufferResource> m_Buffers;

    TextureHandle m_Backbuffer;
    ResourceHandle m_NextHandle = 1;

    bool m_Compiled = false;
};

} // namespace happycat
