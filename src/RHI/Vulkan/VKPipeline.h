#pragma once

#include "VKCommon.h"
#include "RHI/RHITypes.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace happycat {

class VKDevice;
class VKShaderModule;
class VKDescriptorSetLayout;

struct PipelineLayoutDesc {
    std::vector<VKDescriptorSetLayout*> setLayouts;
    std::vector<VkPushConstantRange> pushConstantRanges;
    const char* debugName = nullptr;
};

class VKPipelineLayout {
public:
    static std::unique_ptr<VKPipelineLayout> Create(
        VKDevice* device,
        const PipelineLayoutDesc& desc);
    ~VKPipelineLayout();

    VKPipelineLayout(const VKPipelineLayout&) = delete;
    VKPipelineLayout& operator=(const VKPipelineLayout&) = delete;

    VkPipelineLayout GetHandle() const { return m_Layout; }

private:
    VKPipelineLayout() = default;
    bool Initialize(VKDevice* device, const PipelineLayoutDesc& desc);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
};

class VKPipeline {
public:
    static std::unique_ptr<VKPipeline> Create(
        VKDevice* device,
        VkRenderPass renderPass,
        const GraphicsPipelineDesc& desc,
        VKPipelineLayout* pipelineLayout = nullptr);

    ~VKPipeline();

    VKPipeline(const VKPipeline&) = delete;
    VKPipeline& operator=(const VKPipeline&) = delete;

    VkPipeline GetHandle() const { return m_Pipeline; }
    VkPipelineLayout GetLayout() const { return m_Layout; }

private:
    VKPipeline() = default;
    bool Initialize(VKDevice* device, VkRenderPass renderPass, const GraphicsPipelineDesc& desc, VKPipelineLayout* pipelineLayout);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    bool m_OwnsLayout = false;
};

} // namespace happycat
