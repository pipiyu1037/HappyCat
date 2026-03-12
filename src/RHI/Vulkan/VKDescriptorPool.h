#pragma once

#include "VKCommon.h"
#include "VKDescriptorSetLayout.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace happycat {

class VKDevice;
class VKDescriptorSet;
class VKDescriptorSetLayout;

struct DescriptorPoolSize {
    DescriptorType type = DescriptorType::UniformBuffer;
    u32 count = 1;
};

struct DescriptorPoolDesc {
    std::vector<DescriptorPoolSize> poolSizes;
    u32 maxSets = 100;
    bool allowFree = true;
    const char* debugName = nullptr;
};

class VKDescriptorPool {
public:
    static std::unique_ptr<VKDescriptorPool> Create(
        VKDevice* device,
        const DescriptorPoolDesc& desc);

    ~VKDescriptorPool();

    VKDescriptorPool(const VKDescriptorPool&) = delete;
    VKDescriptorPool& operator=(const VKDescriptorPool&) = delete;

    VkDescriptorPool GetHandle() const { return m_Pool; }

    // Allocate a descriptor set from a layout
    std::unique_ptr<VKDescriptorSet> Allocate(VKDescriptorSetLayout* layout);

    // Allocate multiple descriptor sets
    bool AllocateSets(
        const std::vector<VKDescriptorSetLayout*>& layouts,
        std::vector<VkDescriptorSet>& outSets);

    // Free descriptor sets (if pool supports it void FreeSets(const std::vector<VkDescriptorSet>& sets);

    // Reset pool (recycles all sets)
    void Reset();

private:
    VKDescriptorPool() = default;
    bool Initialize(VKDevice* device, const DescriptorPoolDesc& desc);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkDescriptorPool m_Pool = VK_NULL_HANDLE;
    DescriptorPoolDesc m_Desc;
};

} // namespace happycat
