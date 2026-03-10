#pragma once

#include "Core/Utils/Types.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace happycat {

class VKDevice;
class VKSemaphore;
class VKFence;
class VKCommandPool;
class VKCommandBuffer;

// Forward declaration for VKCommandBuffer wrapper
class VKCommandBuffer;

class FrameContext {
public:
    // Create with enough resources for both frames in flight and swapchain images
    FrameContext(VKDevice* device, u32 framesInFlight, u32 swapchainImageCount);
    ~FrameContext();

    // Non-copyable
    FrameContext(const FrameContext&) = delete;
    FrameContext& operator=(const FrameContext&) = delete;

    // Frame management
    void BeginFrame();
    void EndFrame();

    // Getters
    u32 GetCurrentFrameIndex() const { return m_CurrentFrame; }
    u64 GetFrameNumber() const { return m_FrameNumber; }
    u32 GetFramesInFlight() const { return m_FramesInFlight; }

    // Synchronization - indexed by swapchain image for both semaphores
    VkSemaphore GetImageAvailableSemaphore(u32 imageIndex) const;
    VkSemaphore GetRenderFinishedSemaphore(u32 imageIndex) const;
    VkFence GetRenderFence() const;

    // Command buffer
    VKCommandBuffer* GetCurrentCommandBuffer() const;

private:
    struct FrameData {
        std::unique_ptr<VKFence> renderFence;
        std::unique_ptr<VKCommandPool> commandPool;
        std::unique_ptr<VKCommandBuffer> commandBuffer;
    };

    VKDevice* m_Device;
    u32 m_FramesInFlight;
    u32 m_SwapchainImageCount;
    u32 m_CurrentFrame = 0;
    u64 m_FrameNumber = 0;

    std::vector<FrameData> m_FrameData;

    // Per-swapchain-image semaphores for acquire (prevents reuse issues)
    std::vector<std::unique_ptr<VKSemaphore>> m_ImageAvailableSemaphores;

    // Per-swapchain-image render finished semaphores (prevents reuse issues with present)
    std::vector<std::unique_ptr<VKSemaphore>> m_RenderFinishedSemaphores;
};

} // namespace happycat
