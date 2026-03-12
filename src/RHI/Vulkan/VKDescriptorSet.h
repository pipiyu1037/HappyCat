#pragma once

#include "VKCommon.h"
#include "VKDescriptorSetLayout.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace happycat {

class VKDevice;
class VKBuffer;
class VKImageView;
class VKSampler;

struct DescriptorBufferInfo {
    VKBuffer* buffer = nullptr;
    VkDeviceSize offset = 0;
    VkDeviceSize range = VK_WHOLE_SIZE;
};

struct DescriptorImageInfo {
    VKSampler* sampler = nullptr;
    VKImageView* imageView = nullptr;
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
};

struct DescriptorWrite {
    u32 dstBinding = 0;
    u32 dstArrayElement = 0;
    DescriptorType descriptorType = DescriptorType::UniformBuffer;

    std::vector<DescriptorBufferInfo> bufferInfos;
    std::vector<DescriptorImageInfo> imageInfos;
};

class VKDescriptorSet {
public:
    // Sets are created through VKDescriptorPool::Allocate
    VKDescriptorSet(VkDescriptorSet set, VkDescriptorSetLayout layout);
    ~VKDescriptorSet() = default;

    VKDescriptorSet(const VKDescriptorSet&) = delete;
    VKDescriptorSet& operator=(const VKDescriptorSet&) = delete;
    VKDescriptorSet(VKDescriptorSet&&) = default;
    VKDescriptorSet& operator=(VKDescriptorSet&&) = default;

    VkDescriptorSet GetHandle() const { return m_Set; }
    VkDescriptorSetLayout GetLayout() const { return m_Layout; }

    // Update descriptor set with writes
    void Update(VKDevice* device, const std::vector<DescriptorWrite>& writes);

    // Convenience methods for single updates
    void WriteUniformBuffer(
        VKDevice* device,
        u32 binding,
        VKBuffer* buffer,
        VkDeviceSize offset = 0,
        VkDeviceSize range = VK_WHOLE_SIZE);

    void WriteCombinedImageSampler(
        VKDevice* device,
        u32 binding,
        VKImageView* imageView,
        VKSampler* sampler,
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

private:
    VkDescriptorSet m_Set = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
};

} // namespace happycat
