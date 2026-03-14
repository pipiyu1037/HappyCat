#include "PBRMaterialApp.h"
#include "Renderer/Material/Material.h"
#include "Renderer/Material/Texture2D.h"
#include "Renderer/Material/MaterialTypes.h"
#include "RHI/Vulkan/VKDevice.h"
#include "RHI/Vulkan/VKSwapChain.h"
#include "RHI/Vulkan/VKCommandBuffer.h"
#include "RHI/Vulkan/VKDescriptorPool.h"
#include "RHI/Vulkan/VKDescriptorSet.h"
#include "RHI/Vulkan/VKBuffer.h"
#include "Engine/FrameContext.h"
#include "Core/Utils/Logger.h"
#include "Renderer/Shader/ShaderCompiler.h"
#include "Core/Math/MathTypes.h"

#include <cstring>
#include <vector>

namespace happycat {

// Define PI constant
constexpr float PI = 3.14159265359f;

// Vertex structure with tangent and bitangent for normal mapping
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(Vertex);
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDesc;
    }

    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescs(5);

        // Position
        attributeDescs[0].binding = 0;
        attributeDescs[0].location = 0;
        attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[0].offset = offsetof(Vertex, pos);

        // Normal
        attributeDescs[1].binding = 0;
        attributeDescs[1].location = 1;
        attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[1].offset = offsetof(Vertex, normal);

        // TexCoord
        attributeDescs[2].binding = 0;
        attributeDescs[2].location = 2;
        attributeDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescs[2].offset = offsetof(Vertex, texCoord);

        // Tangent
        attributeDescs[3].binding = 0;
        attributeDescs[3].location = 3;
        attributeDescs[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[3].offset = offsetof(Vertex, tangent);

        // Bitangent
        attributeDescs[4].binding = 0;
        attributeDescs[4].location = 4;
        attributeDescs[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescs[4].offset = offsetof(Vertex, bitangent);

        return attributeDescs;
    }
};

// Helper to create a sphere mesh
static void CreateSphere(std::vector<Vertex>& vertices, std::vector<u32>& indices, float radius, u32 segments, u32 rings) {
    vertices.clear();
    indices.clear();

    for (u32 ring = 0; ring <= rings; ++ring) {
        float phi = PI * float(ring) / float(rings);
        float y = cos(phi) * radius;
        float ringRadius = sin(phi) * radius;

        for (u32 seg = 0; seg <= segments; ++seg) {
            float theta = 2.0f * PI * float(seg) / float(segments);
            float x = cos(theta) * ringRadius;
            float z = sin(theta) * ringRadius;

            Vertex v;
            v.pos = glm::vec3(x, y, z);
            v.normal = glm::normalize(v.pos);
            v.texCoord = glm::vec2(float(seg) / float(segments), float(ring) / float(rings));

            // Calculate tangent and bitangent
            glm::vec3 tangent = glm::normalize(glm::vec3(-sin(theta), 0, cos(theta)));
            glm::vec3 bitangent = glm::cross(v.normal, tangent);
            v.tangent = tangent;
            v.bitangent = bitangent;

            vertices.push_back(v);
        }
    }

    for (u32 ring = 0; ring < rings; ++ring) {
        for (u32 seg = 0; seg < segments; ++seg) {
            u32 current = ring * (segments + 1) + seg;
            u32 next = current + segments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
}

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

PBRMaterialApp::PBRMaterialApp(const ApplicationConfig& config)
    : Application(config) {}

bool PBRMaterialApp::OnInit() {
    if (!CreateRenderPass()) return false;
    if (!CreateDescriptorResources()) return false;
    if (!CreateMaterial()) return false;  // Must be before CreatePipeline for descriptor layout
    if (!CreatePipeline()) return false;
    if (!CreateGeometry()) return false;

    HC_CORE_INFO("PBR Material app initialized");
    return true;
}

void PBRMaterialApp::OnShutdown() {
    VkDevice device = GetDevice()->GetHandle();
    vkDeviceWaitIdle(device);

    m_Material.reset();
    m_IndexBuffer.reset();
    m_VertexBuffer.reset();
    m_SceneDescriptorSets.clear();
    m_SceneUniformBuffers.clear();
    m_SceneDescriptorPool.reset();
    m_SceneDescriptorSetLayout.reset();

    if (m_Pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }
    if (m_PipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }

    for (auto& fb : m_Framebuffers) {
        if (fb) vkDestroyFramebuffer(device, fb, nullptr);
    }
    if (m_RenderPass) vkDestroyRenderPass(device, m_RenderPass, nullptr);

    HC_CORE_INFO("PBR Material app shutdown");
}

void PBRMaterialApp::OnUpdate(f32 deltaTime) {
    m_Rotation += deltaTime * 0.3f;

    // Simple camera rotation
    m_CameraRotationY += deltaTime * 0.5f;
}

void PBRMaterialApp::OnRender() {
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

    UpdateSceneUniformBuffer(frameIndex);
    m_Material->UpdateUniformBuffer(frameIndex);
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
    VkClearValue clear = {{{0.05f, 0.05f, 0.1f, 1.0f}}};
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clear;
    vkCmdBeginRenderPass(vkCmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind pipeline
    vkCmdBindPipeline(vkCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    // Bind scene descriptor set (set 0)
    VkDescriptorSet sceneDS = m_SceneDescriptorSets[frameIndex]->GetHandle();
    vkCmdBindDescriptorSets(vkCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_PipelineLayout, 0, 1, &sceneDS, 0, nullptr);

    // Bind material descriptor set (set 1)
    VkDescriptorSet materialDS = m_Material->GetDescriptorSet(frameIndex)->GetHandle();
    vkCmdBindDescriptorSets(vkCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_PipelineLayout, 1, 1, &materialDS, 0, nullptr);

    // Bind vertex and index buffers
    VkBuffer vbs[] = {m_VertexBuffer->GetHandle()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(vkCmd, 0, 1, vbs, offsets);
    vkCmdBindIndexBuffer(vkCmd, m_IndexBuffer->GetHandle(), 0, VK_INDEX_TYPE_UINT32);

    // Set viewport and scissor
    VkViewport viewport{0, 0, (f32)swapChain->GetExtent().width, (f32)swapChain->GetExtent().height, 0, 1};
    vkCmdSetViewport(vkCmd, 0, 1, &viewport);
    VkRect2D scissor{{0, 0}, swapChain->GetExtent()};
    vkCmdSetScissor(vkCmd, 0, 1, &scissor);

    // Push constants for model matrix
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), m_Rotation, glm::vec3(0, 1, 0));
    vkCmdPushConstants(vkCmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(glm::mat4), &model);

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

void PBRMaterialApp::OnResize(u32 width, u32 height) {
    GetDevice()->WaitIdle();
    for (auto& fb : m_Framebuffers) {
        if (fb) vkDestroyFramebuffer(GetDevice()->GetHandle(), fb, nullptr);
    }
    m_Framebuffers.clear();
    HC_CORE_INFO("PBRMaterialApp resized to {0}x{1}", width, height);
}

bool PBRMaterialApp::CreateRenderPass() {
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

bool PBRMaterialApp::CreateDescriptorResources() {
    auto device = GetDevice();
    u32 framesInFlight = GetFrameContext()->GetFramesInFlight();

    // Create scene descriptor set layout (set 0)
    DescriptorSetLayoutDesc sceneLayoutDesc{};
    sceneLayoutDesc.bindings = {{0, DescriptorType::UniformBuffer, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT}};
    m_SceneDescriptorSetLayout = VKDescriptorSetLayout::Create(device, sceneLayoutDesc);
    if (!m_SceneDescriptorSetLayout) return false;

    // Create scene descriptor pool
    DescriptorPoolDesc poolDesc{};
    poolDesc.poolSizes = {{DescriptorType::UniformBuffer, framesInFlight}};
    poolDesc.maxSets = framesInFlight;
    m_SceneDescriptorPool = VKDescriptorPool::Create(device, poolDesc);
    if (!m_SceneDescriptorPool) return false;

    // Create uniform buffers and descriptor sets
    m_SceneUniformBuffers.resize(framesInFlight);
    m_SceneDescriptorSets.resize(framesInFlight);

    for (u32 i = 0; i < framesInFlight; ++i) {
        m_SceneUniformBuffers[i] = VKBuffer::Create(
            device, sizeof(SceneData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (!m_SceneUniformBuffers[i]) return false;

        m_SceneDescriptorSets[i] = m_SceneDescriptorPool->Allocate(m_SceneDescriptorSetLayout.get());
        if (!m_SceneDescriptorSets[i]) return false;

        m_SceneDescriptorSets[i]->WriteUniformBuffer(device, 0, m_SceneUniformBuffers[i].get());
    }
    return true;
}

bool PBRMaterialApp::CreatePipeline() {
    auto device = GetDevice();
    VkDevice vkDevice = device->GetHandle();

    // Create pipeline layout with two descriptor sets
    VkDescriptorSetLayout setLayouts[] = {
        m_SceneDescriptorSetLayout->GetHandle(),
        m_Material->GetDescriptorSetLayout()->GetHandle()
    };

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 2;
    layoutInfo.pSetLayouts = setLayouts;

    // Push constant range for model matrix
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VK_CHECK(vkCreatePipelineLayout(vkDevice, &layoutInfo, nullptr, &m_PipelineLayout));

    // Load shaders
    auto vertSpirv = LoadOrCompileShader("shaders/pbr/pbr.vert", ShaderStage::Vertex);
    auto fragSpirv = LoadOrCompileShader("shaders/pbr/pbr.frag", ShaderStage::Fragment);

    if (vertSpirv.empty() || fragSpirv.empty()) {
        HC_CORE_ERROR("Failed to load/compile PBR shaders");
        return false;
    }

    VkShaderModule vertModule = CreateShaderModuleFromSpirv(vkDevice, vertSpirv);
    VkShaderModule fragModule = CreateShaderModuleFromSpirv(vkDevice, fragSpirv);
    if (!vertModule || !fragModule) {
        HC_CORE_ERROR("Failed to create shader modules");
        return false;
    }

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

    // Dynamic state
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Vertex input
    auto binding = Vertex::GetBindingDescription();
    auto attrDescs = Vertex::GetAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
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
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = m_RenderPass;
    pipelineInfo.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);

    vkDestroyShaderModule(vkDevice, vertModule, nullptr);
    vkDestroyShaderModule(vkDevice, fragModule, nullptr);

    if (result != VK_SUCCESS) {
        HC_CORE_ERROR("Failed to create graphics pipeline");
        return false;
    }

    HC_CORE_INFO("PBR pipeline created successfully");
    return true;
}

bool PBRMaterialApp::CreateGeometry() {
    auto device = GetDevice();

    // Create sphere mesh
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    CreateSphere(vertices, indices, 1.0f, 32, 24);

    m_IndexCount = static_cast<u32>(indices.size());

    // Vertex buffer
    VkDeviceSize vbSize = sizeof(vertices[0]) * vertices.size();
    auto stagingVB = VKBuffer::Create(device, vbSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* vbData = stagingVB->Map();
    memcpy(vbData, vertices.data(), (size_t)vbSize);
    stagingVB->Unmap();

    m_VertexBuffer = VKBuffer::Create(device, vbSize,
                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CopyBufferImmediate(device, stagingVB.get(), m_VertexBuffer.get(), vbSize);

    // Index buffer
    VkDeviceSize ibSize = sizeof(indices[0]) * indices.size();
    auto stagingIB = VKBuffer::Create(device, ibSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* ibData = stagingIB->Map();
    memcpy(ibData, indices.data(), (size_t)ibSize);
    stagingIB->Unmap();

    m_IndexBuffer = VKBuffer::Create(device, ibSize,
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CopyBufferImmediate(device, stagingIB.get(), m_IndexBuffer.get(), ibSize);

    HC_CORE_INFO("Created sphere geometry with {0} vertices, {1} indices", vertices.size(), indices.size());
    return true;
}

bool PBRMaterialApp::CreateMaterial() {
    auto device = GetDevice();
    HC_CORE_INFO("CreateMaterial: Starting...");

    // Create a simple PBR material
    m_Material = Material::Create(device, "PBR_Sphere");
    if (!m_Material) {
        HC_CORE_ERROR("Failed to create material");
        return false;
    }
    HC_CORE_INFO("CreateMaterial: Material created");

    // Set material parameters
    PBRMaterialParams params{};
    params.albedoColor = glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);  // Red
    params.metallic = 0.8f;
    params.roughness = 0.3f;
    params.ao = 1.0f;
    m_Material->SetParameters(params);
    HC_CORE_INFO("CreateMaterial: Parameters set");
    HC_CORE_INFO("CreateMaterial: Parameters set");

    // Create descriptor pool for material
    u32 framesInFlight = GetFrameContext()->GetFramesInFlight();
    HC_CORE_INFO("CreateMaterial: framesInFlight = {0}", framesInFlight);

    if (!m_Material->CreateDescriptorPool(framesInFlight)) {
        HC_CORE_ERROR("Failed to create material descriptor pool");
        return false;
    }

    HC_CORE_INFO("Created PBR material");
    return true;
}

void PBRMaterialApp::CreateFramebufferIfNeeded(u32 imageIndex) {
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

void PBRMaterialApp::UpdateSceneUniformBuffer(u32 frameIndex) {
    f32 aspect = (f32)GetSwapChain()->GetExtent().width / (f32)GetSwapChain()->GetExtent().height;

    // Calculate camera position
    float camX = sin(m_CameraRotationY) * m_CameraDistance;
    float camZ = cos(m_CameraRotationY) * m_CameraDistance;
    glm::vec3 cameraPos(camX, 1.5f, camZ);

    SceneData sceneData{};
    sceneData.view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    sceneData.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    sceneData.projection[1][1] *= -1;
    sceneData.cameraPos = cameraPos;

    void* data = m_SceneUniformBuffers[frameIndex]->Map();
    memcpy(data, &sceneData, sizeof(sceneData));
    m_SceneUniformBuffers[frameIndex]->Unmap();
}

std::vector<u32> PBRMaterialApp::LoadOrCompileShader(const std::string& basePath, ShaderStage stage) {
    std::string spvPath = basePath + ".spv";
    auto spirv = ShaderCompiler::LoadSpirV(spvPath);
    if (!spirv.empty()) {
        HC_CORE_INFO("Loaded precompiled shader: {0}", spvPath);
        return spirv;
    }

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

VkShaderModule PBRMaterialApp::CreateShaderModuleFromSpirv(VkDevice device, const std::vector<u32>& spirv) {
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
