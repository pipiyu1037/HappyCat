#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace happycat {

class VKDevice;

enum class DescriptorType : u32 {
    UniformBuffer = 0,
    CombinedImageSampler = 1,
    StorageBuffer = 2,
    StorageImage = 3,
    InputAttachment = 4,
    Count
};

inline VkDescriptorType ToVkDescriptorType(DescriptorType type) {
    switch (type) {
        case DescriptorType::UniformBuffer:        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescriptorType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case DescriptorType::StorageBuffer:        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DescriptorType::StorageImage:         return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case DescriptorType::InputAttachment:     return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        default:                                   return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
}

struct DescriptorBinding {
    u32 binding = 0;
    DescriptorType type = DescriptorType::UniformBuffer;
    u32 count = 1;
    VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
    const VkSampler* immutableSamplers = nullptr;
};

struct DescriptorSetLayoutDesc {
    std::vector<DescriptorBinding> bindings;
    VkDescriptorSetLayoutCreateFlags flags = 0;
    const char* debugName = nullptr;
};

class VKDescriptorSetLayout {
public:
    static std::unique_ptr<VKDescriptorSetLayout> Create(
        VKDevice* device,
        const DescriptorSetLayoutDesc& desc);

    ~VKDescriptorSetLayout();

    VKDescriptorSetLayout(const VKDescriptorSetLayout&) = delete;
    VKDescriptorSetLayout& operator=(const VKDescriptorSetLayout&) = delete;

    VkDescriptorSetLayout GetHandle() const { return m_Layout; }
    const DescriptorSetLayoutDesc& GetDesc() const { return m_Desc; }

private:
    VKDescriptorSetLayout() = default;
    bool Initialize(VKDevice* device, const DescriptorSetLayoutDesc& desc);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
    DescriptorSetLayoutDesc m_Desc;
};

} // namespace happycat
