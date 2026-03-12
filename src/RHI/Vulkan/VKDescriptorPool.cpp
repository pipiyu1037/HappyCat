#include "VKDescriptorPool.h"
#include "VKDescriptorSet.h"
#include "VKDevice.h"
#include "Core/Utils/ResourceTracker.h"

namespace happycat {

std::unique_ptr<VKDescriptorPool> VKDescriptorPool::Create(
    VKDevice* device,
    const DescriptorPoolDesc& desc)
{
    auto pool = std::unique_ptr<VKDescriptorPool>(new VKDescriptorPool());
    if (!pool->Initialize(device, desc)) {
        HC_CORE_ERROR("Failed to create VKDescriptorPool");
        return nullptr;
    }
    HC_TRACK_RESOURCE_DBG(pool.get(), ResourceType::DescriptorPool,
                          desc.debugName ? desc.debugName : "DescriptorPool", 0);
    return pool;
}

VKDescriptorPool::~VKDescriptorPool() {
    HC_UNTRACK_RESOURCE_DBG(this);
    if (m_Pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_Device, m_Pool, nullptr);
    }
}

bool VKDescriptorPool::Initialize(VKDevice* device, const DescriptorPoolDesc& desc) {
    m_Device = device->GetHandle();
    m_Desc = desc;

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(desc.poolSizes.size());

    for (const auto& size : desc.poolSizes) {
        VkDescriptorPoolSize vkSize{};
        vkSize.type = ToVkDescriptorType(size.type);
        vkSize.descriptorCount = size.count;
        poolSizes.push_back(vkSize);
    }

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.flags = desc.allowFree ? VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT : 0;
    createInfo.maxSets = desc.maxSets;
    createInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
    createInfo.pPoolSizes = poolSizes.data();

    VK_CHECK(vkCreateDescriptorPool(m_Device, &createInfo, nullptr, &m_Pool));
    return true;
}

std::unique_ptr<VKDescriptorSet> VKDescriptorPool::Allocate(VKDescriptorSetLayout* layout) {
    VkDescriptorSetLayout layoutHandle = layout->GetHandle();

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layoutHandle;

    VkDescriptorSet set;
    VkResult result = vkAllocateDescriptorSets(m_Device, &allocInfo, &set);

    if (result != VK_SUCCESS) {
        HC_CORE_ERROR("Failed to allocate descriptor set: {0}", static_cast<i32>(result));
        return nullptr;
    }

    return std::make_unique<VKDescriptorSet>(set, layoutHandle);
}

bool VKDescriptorPool::AllocateSets(
    const std::vector<VKDescriptorSetLayout*>& layouts,
    std::vector<VkDescriptorSet>& outSets)
{
    std::vector<VkDescriptorSetLayout> layoutHandles;
    layoutHandles.reserve(layouts.size());
    for (auto* layout : layouts) {
        layoutHandles.push_back(layout->GetHandle());
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Pool;
    allocInfo.descriptorSetCount = static_cast<u32>(layoutHandles.size());
    allocInfo.pSetLayouts = layoutHandles.data();

    outSets.resize(layouts.size());
    VkResult result = vkAllocateDescriptorSets(m_Device, &allocInfo, outSets.data());

    if (result != VK_SUCCESS) {
        HC_CORE_ERROR("Failed to allocate descriptor sets: {0}", static_cast<i32>(result));
        return false;
    }

    return true;
}

void VKDescriptorPool::FreeSet(VkDescriptorSet set) {
    if (m_Desc.allowFree) {
        vkFreeDescriptorSets(m_Device, m_Pool, 1, &set);
    } else {
        HC_CORE_WARN("Attempted to free individual descriptor set from pool that doesn't support it");
    }
}

void VKDescriptorPool::FreeSets(const std::vector<VkDescriptorSet>& sets) {
    if (m_Desc.allowFree && !sets.empty()) {
        vkFreeDescriptorSets(m_Device, m_Pool,
                            static_cast<u32>(sets.size()), sets.data());
    }
}

void VKDescriptorPool::Reset() {
    vkResetDescriptorPool(m_Device, m_Pool, 0);
}

} // namespace happycat
