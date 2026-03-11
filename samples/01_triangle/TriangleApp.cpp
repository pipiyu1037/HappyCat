#include "TriangleApp.h"
#include "RHI/Vulkan/VKDevice.h"
#include "RHI/Vulkan/VKSwapChain.h"
#include "RHI/Vulkan/VKPipeline.h"
#include "RHI/Vulkan/VKCommandBuffer.h"
#include "Engine/FrameContext.h"
#include "Core/Utils/Logger.h"
#include "Renderer/Shader/ShaderCompiler.h"

#include <fstream>
#include <vector>

namespace happycat {

TriangleApp::TriangleApp(const ApplicationConfig& config)
    : Application(config)
{
}

bool TriangleApp::OnInit() {
    if (!CreateRenderPass()) {
        HC_CORE_ERROR("Failed to create render pass");
        return false;
    }

    HC_CORE_INFO("Triangle app initialized");
    return true;
}

void TriangleApp::OnShutdown() {
    VkDevice device = GetDevice()->GetHandle();

    vkDeviceWaitIdle(device);

    for (auto& fb : m_Framebuffers) {
        if (fb) {
            vkDestroyFramebuffer(device, fb, nullptr);
        }
    }

    if (m_Pipeline) {
        vkDestroyPipeline(device, m_Pipeline, nullptr);
    }
    if (m_PipelineLayout) {
        vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    }
    if (m_RenderPass) {
        vkDestroyRenderPass(device, m_RenderPass, nullptr);
    }

    HC_CORE_INFO("Triangle app shutdown");
}

void TriangleApp::OnUpdate(f32 deltaTime) {
    // Nothing to update
}

void TriangleApp::OnRender() {
    auto device = GetDevice();
    auto swapChain = GetSwapChain();
    auto frameCtx = GetFrameContext();

    frameCtx->BeginFrame();

    // Acquire next image using per-frame semaphore (use frame index for acquire semaphore)
    u32 frameIndex = frameCtx->GetCurrentFrameIndex();
    VkSemaphore imageAvailableSemaphore = frameCtx->GetImageAvailableSemaphore(frameIndex);
    u32 imageIndex = swapChain->AcquireNextImage(imageAvailableSemaphore, VK_NULL_HANDLE);

    if (imageIndex == UINT32_MAX) {
        // Swapchain needs recreation
        frameCtx->EndFrame();
        return;
    }

    // Create framebuffer if needed
    if (m_Framebuffers.size() <= imageIndex || !m_Framebuffers[imageIndex]) {
        m_Framebuffers.resize(swapChain->GetImageCount());

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = m_RenderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &swapChain->GetImageViews()[imageIndex];
        fbInfo.width = swapChain->GetExtent().width;
        fbInfo.height = swapChain->GetExtent().height;
        fbInfo.layers = 1;

        if (m_Framebuffers[imageIndex]) {
            vkDestroyFramebuffer(device->GetHandle(), m_Framebuffers[imageIndex], nullptr);
        }

        vkCreateFramebuffer(device->GetHandle(), &fbInfo, nullptr, &m_Framebuffers[imageIndex]);
    }

    // Record command buffer
    auto cmd = frameCtx->GetCurrentCommandBuffer();
    VkCommandBuffer vkCmd = cmd->GetHandle();
    vkResetCommandBuffer(vkCmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(vkCmd, &beginInfo);

    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_RenderPass;
    renderPassInfo.framebuffer = m_Framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain->GetExtent();

    VkClearValue clearColor = {{{0.0f, 0.0f, 1.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(vkCmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind pipeline
    vkCmdBindPipeline(vkCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    // Set viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(swapChain->GetExtent().width);
    viewport.height = static_cast<f32>(swapChain->GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vkCmd, 0, 1, &viewport);

    // Set scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChain->GetExtent();
    vkCmdSetScissor(vkCmd, 0, 1, &scissor);

    // Draw triangle
    vkCmdDraw(vkCmd, 3, 1, 0, 0);

    // End render pass
    vkCmdEndRenderPass(vkCmd);

    vkEndCommandBuffer(vkCmd);

    // Submit with proper synchronization
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

void TriangleApp::OnResize(u32 width, u32 height) {
    // Wait for device to be idle before destroying framebuffers
    GetDevice()->WaitIdle();

    // Clean up old framebuffers on resize
    VkDevice device = GetDevice()->GetHandle();

    for (auto& fb : m_Framebuffers) {
        if (fb) {
            vkDestroyFramebuffer(device, fb, nullptr);
        }
    }
    m_Framebuffers.clear();

    HC_CORE_INFO("TriangleApp resized to {0}x{1}", width, height);
}

bool TriangleApp::CreateRenderPass() {
    auto device = GetDevice()->GetHandle();
    auto format = GetSwapChain()->GetImageFormat();

    // Create render pass
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    // Subpass dependency
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
        return false;
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        return false;
    }

    // Load shaders - use runtime compilation from GLSL source
    auto vertSpirv = LoadOrCompileShader("shaders/triangle/triangle.vert", ShaderStage::Vertex);
    auto fragSpirv = LoadOrCompileShader("shaders/triangle/triangle.frag", ShaderStage::Fragment);

    if (vertSpirv.empty() || fragSpirv.empty()) {
        HC_CORE_ERROR("Failed to load/compile shaders");
        return false;
    }

    VkShaderModule vertModule = CreateShaderModuleFromSpirv(device, vertSpirv);
    VkShaderModule fragModule = CreateShaderModuleFromSpirv(device, fragSpirv);

    if (!vertModule || !fragModule) {
        HC_CORE_ERROR("Failed to create shader modules");
        return false;
    }
    HC_CORE_INFO("Shaders loaded successfully");

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName = "main";

    // Dynamic states
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Vertex input (empty - vertices hardcoded in shader)
    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = m_RenderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);
        return false;
    }

    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);

    return true;
}

std::vector<u32> TriangleApp::LoadOrCompileShader(const std::string& basePath, ShaderStage stage) {
    // Try loading precompiled SPIR-V first
    std::string spvPath = basePath + ".spv";
    auto spirv = ShaderCompiler::LoadSpirV(spvPath);
    if (!spirv.empty()) {
        HC_CORE_INFO("Loaded precompiled shader: {0}", spvPath);
        return spirv;
    }

    // Fall back to runtime compilation from GLSL source
    HC_CORE_TRACE("No SPIR-V found, compiling from source: {0}", basePath);

    auto* compiler = ShaderCompilerManager::Get();
    if (!compiler) {
        HC_CORE_ERROR("Shader compiler not initialized");
        return {};
    }

    ShaderCompileResult result = compiler->CompileFromFile(basePath, stage);
    if (!result.success) {
        HC_CORE_ERROR("Failed to compile shader '{0}': {1}", basePath, result.errorMessage);
        return {};
    }

    HC_CORE_INFO("Compiled shader: {0} ({1} bytes)", basePath, result.spirv.size() * 4);
    return result.spirv;
}

VkShaderModule TriangleApp::CreateShaderModuleFromSpirv(VkDevice device, const std::vector<u32>& spirv) {
    if (spirv.empty()) return VK_NULL_HANDLE;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(u32);
    createInfo.pCode = spirv.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

} // namespace happycat
