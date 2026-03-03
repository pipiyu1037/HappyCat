#include "VKQueue.h"

namespace happycat {

VKQueue::VKQueue(VkDevice device, u32 familyIndex, u32 queueIndex)
    : m_Device(device), m_FamilyIndex(familyIndex), m_QueueIndex(queueIndex) {
    vkGetDeviceQueue(device, familyIndex, queueIndex, &m_Queue);
}

void VKQueue::Submit(
    const std::vector<VkCommandBuffer>& commandBuffers,
    VkSemaphore waitSemaphore,
    VkSemaphore signalSemaphore,
    VkFence fence)
{
    std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = waitSemaphore != VK_NULL_HANDLE ? 1 : 0;
    submitInfo.pWaitSemaphores = &waitSemaphore;
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = static_cast<u32>(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();
    submitInfo.signalSemaphoreCount = signalSemaphore != VK_NULL_HANDLE ? 1 : 0;
    submitInfo.pSignalSemaphores = &signalSemaphore;

    VK_CHECK(vkQueueSubmit(m_Queue, 1, &submitInfo, fence));
}

void VKQueue::Submit(
    const std::vector<VkCommandBuffer>& commandBuffers,
    const std::vector<VkSemaphore>& waitSemaphores,
    const std::vector<VkPipelineStageFlags>& waitStages,
    const std::vector<VkSemaphore>& signalSemaphores,
    VkFence fence)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = static_cast<u32>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = static_cast<u32>(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();
    submitInfo.signalSemaphoreCount = static_cast<u32>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    VK_CHECK(vkQueueSubmit(m_Queue, 1, &submitInfo, fence));
}

} // namespace happycat
