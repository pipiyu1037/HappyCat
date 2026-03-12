#include "VKDescriptorSet.h"
#include "VKDevice.h"
#include "VKBuffer.h"
#include "VKImageView.h"
#include "VKSampler.h"

namespace happycat {

VKDescriptorSet::VKDescriptorSet(VkDescriptorSet set, VkDescriptorSetLayout layout)
    : m_Set(set), m_Layout(layout)
{
}

void VKDescriptorSet::Update(VKDevice* device, const std::vector<DescriptorWrite>& writes) {
    std::vector<VkWriteDescriptorSet> vkWrites;

    // Temporary storage for buffer/image infos (must persist until vkUpdateDescriptorSets)
    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfoStorage;
    std::vector<std::vector<VkDescriptorImageInfo>> imageInfoStorage;

    bufferInfoStorage.reserve(writes.size());
    imageInfoStorage.reserve(writes.size());

    for (const auto& write : writes) {
        VkWriteDescriptorSet vkWrite{};
        vkWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vkWrite.dstSet = m_Set;
        vkWrite.dstBinding = write.dstBinding;
        vkWrite.dstArrayElement = write.dstArrayElement;
        vkWrite.descriptorCount = static_cast<u32>(
            std::max(write.bufferInfos.size(), write.imageInfos.size()));
        vkWrite.descriptorType = ToVkDescriptorType(write.descriptorType);

        if (!write.bufferInfos.empty()) {
            std::vector<VkDescriptorBufferInfo> bufferInfos;
            bufferInfos.reserve(write.bufferInfos.size());
            for (const auto& info : write.bufferInfos) {
                VkDescriptorBufferInfo vkInfo{};
                vkInfo.buffer = info.buffer ? info.buffer->GetHandle() : VK_NULL_HANDLE;
                vkInfo.offset = info.offset;
                vkInfo.range = info.range;
                bufferInfos.push_back(vkInfo);
            }
            bufferInfoStorage.push_back(std::move(bufferInfos));
            vkWrite.pBufferInfo = bufferInfoStorage.back().data();
        } else {
            bufferInfoStorage.push_back({});
        }

        if (!write.imageInfos.empty()) {
            std::vector<VkDescriptorImageInfo> imageInfos;
            imageInfos.reserve(write.imageInfos.size());
            for (const auto& info : write.imageInfos) {
                VkDescriptorImageInfo vkInfo{};
                vkInfo.sampler = info.sampler ? info.sampler->GetHandle() : VK_NULL_HANDLE;
                vkInfo.imageView = info.imageView ? info.imageView->GetHandle() : VK_NULL_HANDLE;
                vkInfo.imageLayout = info.imageLayout;
                imageInfos.push_back(vkInfo);
            }
            imageInfoStorage.push_back(std::move(imageInfos));
            vkWrite.pImageInfo = imageInfoStorage.back().data();
        } else {
            imageInfoStorage.push_back({});
        }

        vkWrites.push_back(vkWrite);
    }

    vkUpdateDescriptorSets(
        device->GetHandle(),
        static_cast<u32>(vkWrites.size()), vkWrites.data(),
        0, nullptr);
}

void VKDescriptorSet::WriteUniformBuffer(
    VKDevice* device,
    u32 binding,
    VKBuffer* buffer,
    VkDeviceSize offset,
    VkDeviceSize range)
{
    DescriptorWrite write{};
    write.dstBinding = binding;
    write.descriptorType = DescriptorType::UniformBuffer;
    write.bufferInfos.push_back({buffer, offset, range});

    Update(device, {write});
}

void VKDescriptorSet::WriteCombinedImageSampler(
    VKDevice* device,
    u32 binding,
    VKImageView* imageView,
    VKSampler* sampler,
    VkImageLayout imageLayout)
{
    DescriptorWrite write{};
    write.dstBinding = binding;
    write.descriptorType = DescriptorType::CombinedImageSampler;
    write.imageInfos.push_back({sampler, imageView, imageLayout});

    Update(device, {write});
}

} // namespace happycat
