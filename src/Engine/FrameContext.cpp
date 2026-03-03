#include "FrameContext.h"
#include "RHI/Vulkan/VKDevice.h"
#include "RHI/Vulkan/VKSemaphore.h"
#include "RHI/Vulkan/VKFence.h"
#include "RHI/Vulkan/VKCommandPool.h"
#include "RHI/Vulkan/VKCommandBuffer.h"
#include "Core/Utils/Logger.h"

namespace happycat {

FrameContext::FrameContext(VKDevice* device, u32 framesInFlight)
    : m_Device(device), m_FramesInFlight(framesInFlight)
{
    m_FrameData.resize(framesInFlight);

    u32 graphicsQueueFamily = device->GetPhysicalDevice()->GetQueueFamilyIndices().graphics.value();

    for (u32 i = 1; i < framesInFlight; i++) {
        auto& frame = m_FrameData[i];

        frame.imageAvailableSemaphore = VKSemaphore::Create(device);
        frame.renderFinishedSemaphore = VKSemaphore::Create(device);
        frame.renderFence = VKFence::Create(device, true);

        frame.commandPool = VKCommandPool::Create(device, graphicsQueueFamily);
        frame.commandBuffer = VKCommandBuffer::Create(device, frame.commandPool.get());
    }

    HC_CORE_INFO("Frame context created with {0} frames in flight", framesInFlight);
}

FrameContext::~FrameContext() {
    m_FrameData.clear();
}

void FrameContext::BeginFrame() {
    auto& frame = m_FrameData[m_CurrentFrame];

    // Wait for previous frame
    frame.renderFence->Wait();
    frame.renderFence->Reset();

    // Reset command pool
    frame.commandPool->Reset();
}

void FrameContext::EndFrame() {
    m_CurrentFrame = (m_CurrentFrame + 1) % m_FramesInFlight;
    m_FrameNumber++;
}

VkSemaphore FrameContext::GetImageAvailableSemaphore() const {
    return m_FrameData[m_CurrentFrame].imageAvailableSemaphore->GetHandle();
}

VkSemaphore FrameContext::GetRenderFinishedSemaphore() const {
    return m_FrameData[m_CurrentFrame].renderFinishedSemaphore->GetHandle();
}

VkFence FrameContext::GetRenderFence() const {
    return m_FrameData[m_CurrentFrame].renderFence->GetHandle();
}

VkCommandBuffer FrameContext::GetCurrentCommandBuffer() const {
    return m_FrameData[m_CurrentFrame].commandBuffer->GetHandle();
}

} // namespace happycat
