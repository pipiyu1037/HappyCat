#pragma once

#include "Engine/Application.h"
#include "Renderer/Material/Material.h"
#include "RHI/Vulkan/VKBuffer.h"
#include "RHI/Vulkan/VKDescriptorPool.h"
#include "RHI/Vulkan/VKDescriptorSet.h"
#include "RHI/Vulkan/VKDescriptorSetLayout.h"
#include "Core/Math/MathTypes.h"
#include <memory>
#include <vector>

namespace happycat {

class Texture2D;

// PBR Material Demo Application
class PBRMaterialApp : public Application {
public:
    PBRMaterialApp(const ApplicationConfig& config);

protected:
    bool OnInit() override;
    void OnShutdown() override;
    void OnUpdate(f32 deltaTime) override;
    void OnRender() override;
    void OnResize(u32 width, u32 height) override;

private:
    // Scene data uniform buffer
    struct alignas(16) SceneData {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 cameraPos;
        float _padding;
    };

    // Render pass and pipeline
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_Framebuffers;

    // Scene descriptor resources (set 0)
    Scope<VKDescriptorSetLayout> m_SceneDescriptorSetLayout;
    Scope<VKDescriptorPool> m_SceneDescriptorPool;
    std::vector<Scope<VKBuffer>> m_SceneUniformBuffers;
    std::vector<Scope<VKDescriptorSet>> m_SceneDescriptorSets;

    // Material
    Scope<Material> m_Material;

    // Geometry
    Scope<VKBuffer> m_VertexBuffer;
    Scope<VKBuffer> m_IndexBuffer;
    u32 m_IndexCount = 0;

    // Camera
    float m_CameraDistance = 3.0f;
    float m_CameraRotationX = 0.0f;
    float m_CameraRotationY = 0.0f;

    // Animation
    float m_Rotation = 0.0f;

    // Methods
    bool CreateRenderPass();
    bool CreateDescriptorResources();
    bool CreatePipeline();
    bool CreateGeometry();
    bool CreateMaterial();
    void CreateFramebufferIfNeeded(u32 imageIndex);
    void UpdateSceneUniformBuffer(u32 frameIndex);

    // Shader utilities
    std::vector<u32> LoadOrCompileShader(const std::string& basePath, ShaderStage stage);
    VkShaderModule CreateShaderModuleFromSpirv(VkDevice device, const std::vector<u32>& spirv);
};

} // namespace happycat
