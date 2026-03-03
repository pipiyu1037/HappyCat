#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>

namespace happycat {

class VKQueue {
public:
    VKQueue(VkDevice device, u32 familyIndex, u32 queueIndex);
    ~VKQueue() = default;

    // Non-copyable
    VKQueue(const VKQueue&) = delete;
    VKQueue& operator=(const VKQueue&) = delete;

    // Getters
    VkQueue GetHandle() const { return m_Queue; }
    u32 GetFamilyIndex() const { return m_FamilyIndex; }
    u32 GetQueueIndex() const { return m_QueueIndex; }

    // Submit commands
    void Submit(
        const std::vector<VkCommandBuffer>& commandBuffers,
        VkSemaphore waitSemaphore,
        VkSemaphore signalSemaphore,
        VkFence fence);

    void Submit(
        const std::vector<VkCommandBuffer>& commandBuffers,
        const std::vector<VkSemaphore>& waitSemaphores,
        const std::vector<VkPipelineStageFlags>& waitStages,
        const std::vector<VkSemaphore>& signalSemaphores,
        VkFence fence);

private:
    VkDevice m_Device;
    VkQueue m_Queue;
    u32 m_FamilyIndex;
    u32 m_QueueIndex;
};

} // namespace happycat
