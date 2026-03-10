#include "TrianglePass.h"
#include "RHI/Vulkan/VKDevice.h"
#include "RHI/Vulkan/VKSwapChain.h"
#include "RHI/Vulkan/VKCommandBuffer.h"
#include "Core/Utils/Logger.h"

#include <fstream>

namespace happycat {

TrianglePass::TrianglePass(VKDevice* device, VKSwapChain* swapChain)
    : m_Device(device), m_SwapChain(swapChain)
{
}

TrianglePass::~TrianglePass() {
    Cleanup();
}

bool TrianglePass::Initialize() {
    if (!CreateRenderPass()) {
        HC_CORE_ERROR("Failed to create render pass");
        return false;
    }

    if (!CreatePipeline()) {
        HC_CORE_ERROR("Failed to create pipeline");
        return false;
    }

    // Resize framebuffers vector
    m_Framebuffers.resize(m_SwapChain->GetImageCount());

    HC_CORE_INFO("TrianglePass initialized");
    return true;
}

void TrianglePass::Cleanup() {
    VkDevice device = m_Device->GetHandle();

    for (auto& fb : m_Framebuffers) {
        if (fb) {
            vkDestroyFramebuffer(device, fb, nullptr);
        }
    }
    m_Framebuffers.clear();

    if (m_Pipeline) {
        vkDestroyPipeline(device, m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }

    if (m_PipelineLayout) {
        vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }

    if (m_RenderPass) {
        vkDestroyRenderPass(device, m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }
}

void TrianglePass::UpdateFramebuffer(u32 imageIndex, VkImageView imageView) {
    if (m_Framebuffers.size() <= imageIndex) {
        m_Framebuffers.resize(m_SwapChain->GetImageCount());
    }

    if (!m_Framebuffers[imageIndex]) {
        CreateFramebuffer(imageIndex, imageView);
    }
}

bool TrianglePass::CreateRenderPass() {
    VkDevice device = m_Device->GetHandle();
    VkFormat format = m_SwapChain->GetImageFormat();

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

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    return vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass) == VK_SUCCESS;
}

bool TrianglePass::CreatePipeline() {
    VkDevice device = m_Device->GetHandle();

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        HC_CORE_ERROR("Failed to create pipeline layout");
        return false;
    }

    // Load shaders
    auto readFile = [](const std::string& filename) -> std::vector<char> {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            HC_CORE_ERROR("Failed to open shader file: {0}", filename);
            return {};
        }
        size_t size = file.tellg();
        file.seekg(0);
        std::vector<char> buffer(size);
        file.read(buffer.data(), size);
        return buffer;
    };

    auto createShaderModule = [device](const std::vector<char>& code) -> VkShaderModule {
        if (code.empty()) return VK_NULL_HANDLE;

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            return VK_NULL_HANDLE;
        }
        return shaderModule;
    };

    auto vertCode = readFile("shaders/triangle/triangle.vert.spv");
    auto fragCode = readFile("shaders/triangle/triangle.frag.spv");

    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    if (!vertModule || !fragModule) {
        HC_CORE_ERROR("Failed to create shader modules");
        if (vertModule) vkDestroyShaderModule(device, vertModule, nullptr);
        if (fragModule) vkDestroyShaderModule(device, fragModule, nullptr);
        return false;
    }

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

    // Vertex input
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

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);

    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);

    return result == VK_SUCCESS;
}

bool TrianglePass::CreateFramebuffer(u32 imageIndex, VkImageView imageView) {
    VkDevice device = m_Device->GetHandle();

    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = m_RenderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = &imageView;
    fbInfo.width = m_SwapChain->GetExtent().width;
    fbInfo.height = m_SwapChain->GetExtent().height;
    fbInfo.layers = 1;

    if (vkCreateFramebuffer(device, &fbInfo, nullptr, &m_Framebuffers[imageIndex]) != VK_SUCCESS) {
        HC_CORE_ERROR("Failed to create framebuffer for image {0}", imageIndex);
        return false;
    }

    return true;
}

void TrianglePass::Execute(RenderPassContext& ctx, VKCommandBuffer& cmd) {
    VkCommandBuffer vkCmd = cmd.GetHandle();
    VkExtent2D extent = m_SwapChain->GetExtent();

    // Safety check - ensure framebuffer exists
    if (ctx.frameIndex >= m_Framebuffers.size() || m_Framebuffers[ctx.frameIndex] == VK_NULL_HANDLE) {
        HC_CORE_ERROR("TrianglePass::Execute - Framebuffer not ready for index {0}", ctx.frameIndex);
        return;
    }

    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_RenderPass;
    renderPassInfo.framebuffer = m_Framebuffers[ctx.frameIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;

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
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vkCmd, 0, 1, &viewport);

    // Set scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(vkCmd, 0, 1, &scissor);

    // Draw triangle
    vkCmdDraw(vkCmd, 3, 1, 0, 0);

    // End render pass
    vkCmdEndRenderPass(vkCmd);
}

} // namespace happycat
