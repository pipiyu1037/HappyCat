#include "FrameContext.h"
#include "RHI/Vulkan/VKDevice.h"
#include "RHI/Vulkan/VKSemaphore.h"
#include "RHI/Vulkan/VKFence.h"
#include "RHI/Vulkan/VKCommandPool.h"
#include "RHI/Vulkan/VKCommandBuffer.h"
#include "Core/Utils/Logger.h"

namespace happycat {

FrameContext::FrameContext(VKDevice* device, u32 framesInFlight, u32 swapchainImageCount)
    : m_Device(device), m_FramesInFlight(framesInFlight), m_SwapchainImageCount(swapchainImageCount)
{
    m_FrameData.resize(framesInFlight);

    u32 graphicsQueueFamily = device->GetPhysicalDevice()->GetQueueFamilyIndices().graphics.value();

    // Create per-frame resources
    for (u32 i = 0; i < framesInFlight; i++) {
        auto& frame = m_FrameData[i];

        frame.renderFence = VKFence::Create(device, true);
        frame.commandPool = VKCommandPool::Create(device, graphicsQueueFamily);
        frame.commandBuffer = VKCommandBuffer::Create(device, frame.commandPool.get());
    }

    // Create per-swapchain-image semaphores for acquire
    // This ensures each swapchain image has its own semaphore, preventing reuse issues
    m_ImageAvailableSemaphores.resize(swapchainImageCount);
    for (u32 i = 0; i < swapchainImageCount; i++) {
        m_ImageAvailableSemaphores[i] = VKSemaphore::Create(device);
    }

    // Create per-swapchain-image render finished semaphores
    // This prevents reuse issues when presenting
    m_RenderFinishedSemaphores.resize(swapchainImageCount);
    for (u32 i = 0; i < swapchainImageCount; i++) {
        m_RenderFinishedSemaphores[i] = VKSemaphore::Create(device);
    }

    HC_CORE_INFO("Frame context created with {0} frames in flight, {1} swapchain images",
        framesInFlight, swapchainImageCount);
}

FrameContext::~FrameContext() {
    m_RenderFinishedSemaphores.clear();
    m_ImageAvailableSemaphores.clear();
    m_FrameData.clear();
}

void FrameContext::BeginFrame() {
    auto& frame = m_FrameData[m_CurrentFrame];

    // Wait for previous frame to complete
    frame.renderFence->Wait();
    frame.renderFence->Reset();

    // Reset command pool
    frame.commandPool->Reset();
}

void FrameContext::EndFrame() {
    m_CurrentFrame = (m_CurrentFrame + 1) % m_FramesInFlight;
    m_FrameNumber++;
}

VkSemaphore FrameContext::GetImageAvailableSemaphore(u32 imageIndex) const {
    return m_ImageAvailableSemaphores[imageIndex % m_SwapchainImageCount]->GetHandle();
}

VkSemaphore FrameContext::GetRenderFinishedSemaphore(u32 imageIndex) const {
    return m_RenderFinishedSemaphores[imageIndex % m_SwapchainImageCount]->GetHandle();
}

VkFence FrameContext::GetRenderFence() const {
    return m_FrameData[m_CurrentFrame].renderFence->GetHandle();
}

VkCommandBuffer FrameContext::GetCurrentCommandBuffer() const {
    return m_FrameData[m_CurrentFrame].commandBuffer->GetHandle();
}

} // namespace happycat
