#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "Core/Utils/Types.h"
#include "RHI/RHITypes.h"

namespace happycat {

class VKDevice;
class VKSwapChain;
class VKCommandBuffer;

// Triangle rendering pass
class TrianglePass {
public:
    TrianglePass(VKDevice* device, VKSwapChain* swapChain);
    ~TrianglePass();

    // Setup
    bool Initialize();
    void Cleanup();

    // Framebuffer management
    void UpdateFramebuffer(u32 imageIndex, VkImageView imageView);

    // Execute
    bool ShouldExecute() const { return true; }
    void Execute(class RenderPassContext& ctx, VKCommandBuffer& cmd);

private:
    bool CreateRenderPass();
    bool CreatePipeline();
    bool CreateFramebuffer(u32 imageIndex, VkImageView imageView);

    VKDevice* m_Device;
    VKSwapChain* m_SwapChain;

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_Framebuffers;
};

// Render pass context
struct RenderPassContext {
    u32 width;
    u32 height;
    u32 frameIndex;
};

} // namespace happycat
