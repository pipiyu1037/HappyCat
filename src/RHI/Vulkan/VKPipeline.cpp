#include "VKPipeline.h"
#include "VKDevice.h"
#include "VKShaderModule.h"

#include <fstream>
#include <vector>

namespace happycat {

// Helper to read shader file
static std::vector<char> ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        HC_CORE_ERROR("Failed to open shader file: {0}", filename);
        return {};
    }

    size_t fileSize = file.tellg();
    file.seekg(0);
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

std::unique_ptr<VKPipelineLayout> VKPipelineLayout::Create(VKDevice* device) {
    auto layout = std::unique_ptr<VKPipelineLayout>(new VKPipelineLayout());
    if (!layout->Initialize(device)) {
        return nullptr;
    }
    return layout;
}

VKPipelineLayout::~VKPipelineLayout() {
    if (m_Layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_Device, m_Layout, nullptr);
    }
}

bool VKPipelineLayout::Initialize(VKDevice* device) {
    m_Device = device->GetHandle();

    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = 0;
    createInfo.pushConstantRangeCount = 0;

    VK_CHECK(vkCreatePipelineLayout(m_Device, &createInfo, nullptr, &m_Layout));
    return true;
}

std::unique_ptr<VKPipeline> VKPipeline::Create(
    VKDevice* device,
    VkRenderPass renderPass,
    const GraphicsPipelineDesc& desc)
{
    auto pipeline = std::unique_ptr<VKPipeline>(new VKPipeline());
    if (!pipeline->Initialize(device, renderPass, desc)) {
        return nullptr;
    }
    return pipeline;
}

VKPipeline::~VKPipeline() {
    if (m_Pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
    }
    if (m_Layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_Device, m_Layout, nullptr);
    }
}

bool VKPipeline::Initialize(VKDevice* device, VkRenderPass renderPass, const GraphicsPipelineDesc& desc) {
    m_Device = device->GetHandle();

    // Read shaders
    auto vertCode = ReadFile(desc.vertexShader);
    auto fragCode = ReadFile(desc.fragmentShader);

    if (vertCode.empty() || fragCode.empty()) {
        return false;
    }

    // Create shader modules
    VkShaderModuleCreateInfo vertModuleInfo{};
    vertModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertModuleInfo.codeSize = vertCode.size();
    vertModuleInfo.pCode = reinterpret_cast<const u32*>(vertCode.data());

    VkShaderModule vertModule;
    VK_CHECK(vkCreateShaderModule(m_Device, &vertModuleInfo, nullptr, &vertModule));

    VkShaderModuleCreateInfo fragModuleInfo{};
    fragModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragModuleInfo.codeSize = fragCode.size();
    fragModuleInfo.pCode = reinterpret_cast<const u32*>(fragCode.data());

    VkShaderModule fragModule;
    VK_CHECK(vkCreateShaderModule(m_Device, &fragModuleInfo, nullptr, &fragModule));

    // Shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName = "main";

    // Vertex input state (empty for hardcoded vertices)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = desc.topology;
    inputAssembly.primitiveRestartEnable = desc.primitiveRestartEnable;

    // Viewport and scissor
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = desc.polygonMode;
    rasterizer.cullMode = desc.cullMode;
    rasterizer.frontFace = desc.frontFace;
    rasterizer.lineWidth = desc.lineWidth;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = desc.blendEnable;
    colorBlendAttachment.srcColorBlendFactor = desc.srcColorBlendFactor;
    colorBlendAttachment.dstColorBlendFactor = desc.dstColorBlendFactor;
    colorBlendAttachment.colorBlendOp = desc.colorBlendOp;
    colorBlendAttachment.srcAlphaBlendFactor = desc.srcAlphaBlendFactor;
    colorBlendAttachment.dstAlphaBlendFactor = desc.dstAlphaBlendFactor;
    colorBlendAttachment.alphaBlendOp = desc.alphaBlendOp;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Depth/Stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = desc.depthTestEnable;
    depthStencil.depthWriteEnable = desc.depthWriteEnable;
    depthStencil.depthCompareOp = desc.depthCompareOp;

    // Dynamic state
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Pipeline layout
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VK_CHECK(vkCreatePipelineLayout(m_Device, &layoutInfo, nullptr, &m_Layout));

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_Layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = desc.subpass;

    VK_CHECK(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline));

    // Cleanup shader modules
    vkDestroyShaderModule(m_Device, vertModule, nullptr);
    vkDestroyShaderModule(m_Device, fragModule, nullptr);

    return true;
}

} // namespace happycat
