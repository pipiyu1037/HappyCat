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

class FrameContext {
public:
    FrameContext(VKDevice* device, u32 framesInFlight);
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

    // Synchronization
    VkSemaphore GetImageAvailableSemaphore() const;
    VkSemaphore GetRenderFinishedSemaphore() const;
    VkFence GetRenderFence() const;

    // Command buffer
    VkCommandBuffer GetCurrentCommandBuffer() const;

private:
    struct FrameData {
        std::unique_ptr<VKSemaphore> imageAvailableSemaphore;
        std::unique_ptr<VKSemaphore> renderFinishedSemaphore;
        std::unique_ptr<VKFence> renderFence;
        std::unique_ptr<VKCommandPool> commandPool;
        std::unique_ptr<VKCommandBuffer> commandBuffer;
    };

    VKDevice* m_Device;
    u32 m_FramesInFlight;
    u32 m_CurrentFrame = 1;
    u64 m_FrameNumber = 1;

    std::vector<FrameData> m_FrameData;
};

} // namespace happycat
