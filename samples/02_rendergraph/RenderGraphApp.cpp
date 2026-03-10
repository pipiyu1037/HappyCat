#include "RenderGraphApp.h"
#include "TrianglePass.h"
#include "RHI/Vulkan/VKDevice.h"
#include "RHI/Vulkan/VKSwapChain.h"
#include "RHI/Vulkan/VKCommandBuffer.h"
#include "Engine/FrameContext.h"
#include "Core/Utils/Logger.h"

namespace happycat {

RenderGraphApp::RenderGraphApp(const ApplicationConfig& config)
    : Application(config)
{
}

RenderGraphApp::~RenderGraphApp() {
    // OnShutdown is called by Application::Shutdown()
}

bool RenderGraphApp::OnInit() {
    auto device = GetDevice();
    auto swapChain = GetSwapChain();

    // Create triangle pass directly (not using RenderGraph for now)
    m_TrianglePass = std::make_unique<TrianglePass>(device, swapChain);

    if (!m_TrianglePass->Initialize()) {
        HC_CORE_ERROR("Failed to initialize TrianglePass");
        return false;
    }

    HC_CORE_INFO("RenderGraphApp initialized");
    return true;
}

void RenderGraphApp::OnShutdown() {
    GetDevice()->WaitIdle();

    if (m_TrianglePass) {
        m_TrianglePass->Cleanup();
    }

    m_TrianglePass.reset();

    HC_CORE_INFO("RenderGraphApp shutdown");
}

void RenderGraphApp::OnUpdate(f32 deltaTime) {
    // Nothing to update
}

void RenderGraphApp::OnRender() {
    auto device = GetDevice();
    auto swapChain = GetSwapChain();
    auto frameCtx = GetFrameContext();

    frameCtx->BeginFrame();

    // Acquire next image
    u32 frameIndex = frameCtx->GetCurrentFrameIndex();
    VkSemaphore imageAvailableSemaphore = frameCtx->GetImageAvailableSemaphore(frameIndex);
    u32 imageIndex = swapChain->AcquireNextImage(imageAvailableSemaphore, VK_NULL_HANDLE);

    if (imageIndex == UINT32_MAX) {
        // Swapchain needs recreation
        frameCtx->EndFrame();
        return;
    }

    // Update framebuffer for this image
    m_TrianglePass->UpdateFramebuffer(imageIndex, swapChain->GetImageViews()[imageIndex]);

    // Begin command buffer
    auto cmd = frameCtx->GetCurrentCommandBuffer();
    VkCommandBuffer vkCmd = cmd->GetHandle();
    vkResetCommandBuffer(vkCmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(vkCmd, &beginInfo);

    // Execute the triangle pass
    RenderPassContext passCtx{};
    passCtx.width = swapChain->GetExtent().width;
    passCtx.height = swapChain->GetExtent().height;
    passCtx.frameIndex = imageIndex;

    if (m_TrianglePass->ShouldExecute()) {
        m_TrianglePass->Execute(passCtx, *cmd);
    }

    vkEndCommandBuffer(vkCmd);

    // Submit
    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSemaphore renderFinishedSemaphore = frameCtx->GetRenderFinishedSemaphore(imageIndex);
    VkFence renderFence = frameCtx->GetRenderFence();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = &waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

    vkQueueSubmit(device->GetGraphicsQueue()->GetHandle(), 1, &submitInfo, renderFence);

    // Present
    swapChain->Present(device->GetPresentQueue()->GetHandle(), imageIndex, renderFinishedSemaphore);

    frameCtx->EndFrame();
}

void RenderGraphApp::OnResize(u32 width, u32 height) {
    GetDevice()->WaitIdle();

    // Clean up and reinitialize
    if (m_TrianglePass) {
        m_TrianglePass->Cleanup();
        m_TrianglePass->Initialize();
    }

    HC_CORE_INFO("RenderGraphApp resized to {0}x{1}", width, height);
}

} // namespace happycat
