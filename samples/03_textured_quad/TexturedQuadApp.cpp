#include "TexturedQuadApp.h"
#include "RHI/Vulkan/VKDevice.h"
#include "RHI/Vulkan/VKSwapChain.h"
#include "RHI/Vulkan/VKPipeline.h"
#include "RHI/Vulkan/VKCommandBuffer.h"
#include "RHI/Vulkan/VKDescriptorPool.h"
#include "RHI/Vulkan/VKDescriptorSet.h"
#include "Engine/FrameContext.h"
#include "Core/Utils/Logger.h"
#include "Renderer/Shader/ShaderCompiler.h"
#include "Core/Math/MathTypes.h"

#include <cstring>
#include <vector>

namespace happycat {

// Vertex structure
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(Vertex);
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDesc;
    }

    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescs(3);
        attributeDescs[0].binding = 0;
        attributeDescs[0].location = 0;
        attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[0].offset = offsetof(Vertex, pos);

        attributeDescs[1].binding = 0;
        attributeDescs[1].location = 1;
        attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[1].offset = offsetof(Vertex, color);

        attributeDescs[2].binding = 0;
        attributeDescs[2].location = 2;
        attributeDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescs[2].offset = offsetof(Vertex, texCoord);
        return attributeDescs;
    }
};

// Quad data
static const std::vector<Vertex> quadVertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

static const std::vector<u16> quadIndices = {0, 1, 2, 2, 3, 0};

// Helper to copy buffer
static void CopyBufferImmediate(VKDevice* device, VKBuffer* src, VKBuffer* dst, VkDeviceSize size) {
    VkDevice vkDevice = device->GetHandle();
    u32 queueFamily = device->GetGraphicsQueue()->GetFamilyIndex();

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = queueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    VkCommandPool pool;
    vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &pool);

    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(vkDevice, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferCopy region{};
    region.size = size;
    vkCmdCopyBuffer(cmd, src->GetHandle(), dst->GetHandle(), 1, &region);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(device->GetGraphicsQueue()->GetHandle(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device->GetGraphicsQueue()->GetHandle());

    vkFreeCommandBuffers(vkDevice, pool, 1, &cmd);
    vkDestroyCommandPool(vkDevice, pool, nullptr);
}

TexturedQuadApp::TexturedQuadApp(const ApplicationConfig& config)
    : Application(config) {}

bool TexturedQuadApp::OnInit() {
    if (!CreateRenderPass()) return false;
    if (!CreateDescriptorResources()) return false;
    if (!CreatePipeline()) return false;
    if (!CreateBuffers()) return false;

    HC_CORE_INFO("TexturedQuad app initialized");
    return true;
}

void TexturedQuadApp::OnShutdown() {
    VkDevice device = GetDevice()->GetHandle();
    vkDeviceWaitIdle(device);

    m_IndexBuffer.reset();
    m_VertexBuffer.reset();
    m_UniformBuffers.clear();
    m_DescriptorSets.clear();
    m_DescriptorPool.reset();
    m_DescriptorSetLayout.reset();
    m_Pipeline.reset();
    m_PipelineLayout.reset();

    for (auto& fb : m_Framebuffers) {
        if (fb) vkDestroyFramebuffer(device, fb, nullptr);
    }
    if (m_RenderPass) vkDestroyRenderPass(device, m_RenderPass, nullptr);

    HC_CORE_INFO("TexturedQuad app shutdown");
}

void TexturedQuadApp::OnUpdate(f32 deltaTime) {
    m_Rotation += deltaTime * 0.5f;
}

void TexturedQuadApp::OnRender() {
    auto device = GetDevice();
    auto swapChain = GetSwapChain();
    auto frameCtx = GetFrameContext();

    frameCtx->BeginFrame();

    u32 frameIndex = frameCtx->GetCurrentFrameIndex();
    VkSemaphore imageAvailable = frameCtx->GetImageAvailableSemaphore(frameIndex);
    u32 imageIndex = swapChain->AcquireNextImage(imageAvailable, VK_NULL_HANDLE);

    if (imageIndex == UINT32_MAX) {
        frameCtx->EndFrame();
        return;
    }

    UpdateUniformBuffer(frameIndex);
    CreateFramebufferIfNeeded(imageIndex);

    // Record command buffer
    auto cmd = frameCtx->GetCurrentCommandBuffer();
    VkCommandBuffer vkCmd = cmd->GetHandle();
    vkResetCommandBuffer(vkCmd, 0);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(vkCmd, &beginInfo);

    // Begin render pass
    VkRenderPassBeginInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpInfo.renderPass = m_RenderPass;
    rpInfo.framebuffer = m_Framebuffers[imageIndex];
    rpInfo.renderArea.extent = swapChain->GetExtent();
    VkClearValue clear = {{{0.1f, 0.1f, 0.2f, 1.0f}}};
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clear;
    vkCmdBeginRenderPass(vkCmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind pipeline and descriptor set
    vkCmdBindPipeline(vkCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetHandle());
    VkDescriptorSet ds = m_DescriptorSets[frameIndex]->GetHandle();
    vkCmdBindDescriptorSets(vkCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_PipelineLayout->GetHandle(), 0, 1, &ds, 0, nullptr);

    // Bind vertex and index buffers
    VkBuffer vbs[] = {m_VertexBuffer->GetHandle()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(vkCmd, 0, 1, vbs, offsets);
    vkCmdBindIndexBuffer(vkCmd, m_IndexBuffer->GetHandle(), 0, VK_INDEX_TYPE_UINT16);

    // Set viewport and scissor
    VkViewport viewport{0, 0, (f32)swapChain->GetExtent().width, (f32)swapChain->GetExtent().height, 0, 1};
    vkCmdSetViewport(vkCmd, 0, 1, &viewport);
    VkRect2D scissor{{0, 0}, swapChain->GetExtent()};
    vkCmdSetScissor(vkCmd, 0, 1, &scissor);

    // Draw
    vkCmdDrawIndexed(vkCmd, m_IndexCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(vkCmd);
    vkEndCommandBuffer(vkCmd);

    // Submit
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSemaphore renderFinished = frameCtx->GetRenderFinishedSemaphore(imageIndex);
    VkFence fence = frameCtx->GetRenderFence();

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailable;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinished;
    vkQueueSubmit(device->GetGraphicsQueue()->GetHandle(), 1, &submitInfo, fence);

    // Present
    swapChain->Present(device->GetPresentQueue()->GetHandle(), imageIndex, renderFinished);
    frameCtx->EndFrame();
}

void TexturedQuadApp::OnResize(u32 width, u32 height) {
    GetDevice()->WaitIdle();
    for (auto& fb : m_Framebuffers) {
        if (fb) vkDestroyFramebuffer(GetDevice()->GetHandle(), fb, nullptr);
    }
    m_Framebuffers.clear();
    HC_CORE_INFO("TexturedQuadApp resized to {0}x{1}", width, height);
}

bool TexturedQuadApp::CreateRenderPass() {
    VkAttachmentDescription colorAtt{};
    colorAtt.format = GetSwapChain()->GetImageFormat();
    colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    info.attachmentCount = 1;
    info.pAttachments = &colorAtt;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dep;

    return vkCreateRenderPass(GetDevice()->GetHandle(), &info, nullptr, &m_RenderPass) == VK_SUCCESS;
}

bool TexturedQuadApp::CreateDescriptorResources() {
    auto device = GetDevice();
    u32 framesInFlight = GetFrameContext()->GetFramesInFlight();

    // Create descriptor set layout
    DescriptorSetLayoutDesc layoutDesc{};
    layoutDesc.bindings = {{0, DescriptorType::UniformBuffer, 1, VK_SHADER_STAGE_VERTEX_BIT}};
    m_DescriptorSetLayout = VKDescriptorSetLayout::Create(device, layoutDesc);
    if (!m_DescriptorSetLayout) return false;

    // Create descriptor pool
    DescriptorPoolDesc poolDesc{};
    poolDesc.poolSizes = {{DescriptorType::UniformBuffer, framesInFlight}};
    poolDesc.maxSets = framesInFlight;
    m_DescriptorPool = VKDescriptorPool::Create(device, poolDesc);
    if (!m_DescriptorPool) return false;

    // Create uniform buffers and descriptor sets
    m_UniformBuffers.resize(framesInFlight);
    m_DescriptorSets.resize(framesInFlight);

    for (u32 i = 0; i < framesInFlight; ++i) {
        m_UniformBuffers[i] = VKBuffer::Create(
            device, sizeof(UniformBufferObject),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (!m_UniformBuffers[i]) return false;

        m_DescriptorSets[i] = m_DescriptorPool->Allocate(m_DescriptorSetLayout.get());
        if (!m_DescriptorSets[i]) return false;

        m_DescriptorSets[i]->WriteUniformBuffer(device, 0, m_UniformBuffers[i].get());
    }
    return true;
}

bool TexturedQuadApp::CreatePipeline() {
    auto device = GetDevice();

    // Create pipeline layout
    PipelineLayoutDesc layoutDesc{};
    layoutDesc.setLayouts = {m_DescriptorSetLayout.get()};
    m_PipelineLayout = VKPipelineLayout::Create(device, layoutDesc);
    if (!m_PipelineLayout) return false;

    // Load shaders using runtime compiler
    auto vertSpirv = LoadOrCompileShader("shaders/textured_quad/textured_quad.vert", ShaderStage::Vertex);
    auto fragSpirv = LoadOrCompileShader("shaders/textured_quad/textured_quad.frag", ShaderStage::Fragment);

    if (vertSpirv.empty() || fragSpirv.empty()) {
        HC_CORE_ERROR("Failed to load/compile shaders");
        return false;
    }

    VkDevice vkDevice = device->GetHandle();
    VkShaderModule vertModule = CreateShaderModuleFromSpirv(vkDevice, vertSpirv);
    VkShaderModule fragModule = CreateShaderModuleFromSpirv(vkDevice, fragSpirv);
    if (!vertModule || !fragModule) {
        HC_CORE_ERROR("Failed to create shader modules");
        return false;
    }
    // Create pipeline with shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName = "main";
    // Dynamic state
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
    // Vertex input state
    auto binding = Vertex::GetBindingDescription();
    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    auto attrDescs = Vertex::GetAttributeDescriptions();
    vertexInput.vertexAttributeDescriptionCount = static_cast<u32>(attrDescs.size());
    vertexInput.pVertexAttributeDescriptions = attrDescs.data();
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
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
    // Create pipeline
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
    pipelineInfo.layout = m_PipelineLayout->GetHandle();
    pipelineInfo.renderPass = m_RenderPass;
    pipelineInfo.subpass = 0;
    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
    // Cleanup
    vkDestroyShaderModule(vkDevice, vertModule, nullptr);
    vkDestroyShaderModule(vkDevice, fragModule, nullptr);
    return m_Pipeline != nullptr;
}

bool TexturedQuadApp::CreateBuffers() {
    auto device = GetDevice();

    // Vertex buffer
    VkDeviceSize vbSize = sizeof(quadVertices[0]) * quadVertices.size();
    auto stagingVB = VKBuffer::Create(device, vbSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* vbData = stagingVB->Map();
    memcpy(vbData, quadVertices.data(), (size_t)vbSize);
    stagingVB->Unmap();

    m_VertexBuffer = VKBuffer::Create(device, vbSize,
                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CopyBufferImmediate(device, stagingVB.get(), m_VertexBuffer.get(), vbSize);

    // Index buffer
    m_IndexCount = (u32)quadIndices.size();
    VkDeviceSize ibSize = sizeof(quadIndices[0]) * quadIndices.size();
    auto stagingIB = VKBuffer::Create(device, ibSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* ibData = stagingIB->Map();
    memcpy(ibData, quadIndices.data(), (size_t)ibSize);
    stagingIB->Unmap();

    m_IndexBuffer = VKBuffer::Create(device, ibSize,
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CopyBufferImmediate(device, stagingIB.get(), m_IndexBuffer.get(), ibSize);

    return true;
}

void TexturedQuadApp::CreateFramebufferIfNeeded(u32 imageIndex) {
    if (m_Framebuffers.size() <= imageIndex || !m_Framebuffers[imageIndex]) {
        m_Framebuffers.resize(GetSwapChain()->GetImageCount());

        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = m_RenderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &GetSwapChain()->GetImageViews()[imageIndex];
        fbInfo.width = GetSwapChain()->GetExtent().width;
        fbInfo.height = GetSwapChain()->GetExtent().height;
        fbInfo.layers = 1;

        if (m_Framebuffers[imageIndex]) {
            vkDestroyFramebuffer(GetDevice()->GetHandle(), m_Framebuffers[imageIndex], nullptr);
        }
        vkCreateFramebuffer(GetDevice()->GetHandle(), &fbInfo, nullptr, &m_Framebuffers[imageIndex]);
    }
}

void TexturedQuadApp::UpdateUniformBuffer(u32 frameIndex) {
    f32 aspect = (f32)GetSwapChain()->GetExtent().width / (f32)GetSwapChain()->GetExtent().height;

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), m_Rotation, glm::vec3(0, 0, 1));
    ubo.view = glm::lookAt(glm::vec3(2, 2, 2), glm::vec3(0), glm::vec3(0, 0, 1));
    ubo.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    void* data = m_UniformBuffers[frameIndex]->Map();
    memcpy(data, &ubo, sizeof(ubo));
    m_UniformBuffers[frameIndex]->Unmap();
}

std::vector<u32> TexturedQuadApp::LoadOrCompileShader(const std::string& basePath, ShaderStage stage) {
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

VkShaderModule TexturedQuadApp::CreateShaderModuleFromSpirv(VkDevice device, const std::vector<u32>& spirv) {
    if (spirv.empty()) return VK_NULL_HANDLE;

    VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = spirv.size() * sizeof(u32);
    createInfo.pCode = spirv.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

} // namespace happycat
