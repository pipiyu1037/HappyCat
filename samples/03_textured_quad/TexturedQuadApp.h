#pragma once

#include "Engine/Application.h"
#include "RHI/RHITypes.h"
#include "RHI/Vulkan/VKBuffer.h"
#include "RHI/Vulkan/VKDescriptorSetLayout.h"
#include "RHI/Vulkan/VKDescriptorPool.h"
#include "RHI/Vulkan/VKDescriptorSet.h"
#include "RHI/Vulkan/VKPipeline.h"
#include <memory>
#include <vector>

namespace happycat {

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class TexturedQuadApp : public Application {
public:
    explicit TexturedQuadApp(const ApplicationConfig& config);
    ~TexturedQuadApp() = default;

protected:
    bool OnInit() override;
    void OnShutdown() override;
    void OnUpdate(f32 deltaTime) override;
    void OnRender() override;
    void OnResize(u32 width, u32 height) override;

private:
    bool CreateRenderPass();
    bool CreateDescriptorResources();
    bool CreatePipeline();
    bool CreateBuffers();
    void CreateFramebufferIfNeeded(u32 imageIndex);
    void UpdateUniformBuffer(u32 frameIndex);

    static std::vector<u32> LoadOrCompileShader(const std::string& basePath, ShaderStage stage);
    static VkShaderModule CreateShaderModuleFromSpirv(VkDevice device, const std::vector<u32>& spirv);

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_Framebuffers;

    std::unique_ptr<VKPipelineLayout> m_PipelineLayout;
    std::unique_ptr<VKPipeline> m_Pipeline;

    std::unique_ptr<VKDescriptorSetLayout> m_DescriptorSetLayout;
    std::unique_ptr<VKDescriptorPool> m_DescriptorPool;
    std::vector<std::unique_ptr<VKDescriptorSet>> m_DescriptorSets;

    std::vector<std::unique_ptr<VKBuffer>> m_UniformBuffers;
    std::unique_ptr<VKBuffer> m_VertexBuffer;
    std::unique_ptr<VKBuffer> m_IndexBuffer;

    u32 m_IndexCount = 0;
    f32 m_Rotation = 0.0f;
};

} // namespace happycat
