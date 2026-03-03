#pragma once

#include "VKCommon.h"
#include "RHI/RHITypes.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace happycat {

class VKDevice;
class VKShaderModule;

class VKPipelineLayout {
public:
    static std::unique_ptr<VKPipelineLayout> Create(VKDevice* device);
    ~VKPipelineLayout();

    VkPipelineLayout GetHandle() const { return m_Layout; }

private:
    VKPipelineLayout() = default;
    bool Initialize(VKDevice* device);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
};

class VKPipeline {
public:
    static std::unique_ptr<VKPipeline> Create(
        VKDevice* device,
        VkRenderPass renderPass,
        const GraphicsPipelineDesc& desc);

    ~VKPipeline();

    VkPipeline GetHandle() const { return m_Pipeline; }
    VkPipelineLayout GetLayout() const { return m_Layout; }

private:
    VKPipeline() = default;
    bool Initialize(VKDevice* device, VkRenderPass renderPass, const GraphicsPipelineDesc& desc);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
};

} // namespace happycat
