#pragma once

#include "Engine/Application.h"
#include "Renderer/RenderGraph/RenderGraphBuilder.h"
#include <memory>

namespace happycat {

class TrianglePass;

class TriangleApp : public Application {
public:
    explicit TriangleApp(const ApplicationConfig& config);

protected:
    bool OnInit() override;
    void OnShutdown() override;
    void OnUpdate(f32 deltaTime) override;
    void OnRender() override;
    void OnResize(u32 width, u32 height) override;

private:
    bool CreateRenderPass();

    // Helper functions
    static std::vector<char> ReadShaderFile(const std::string& filename);
    static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code);

    // Vulkan objects
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_Framebuffers;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
};

} // namespace happycat
