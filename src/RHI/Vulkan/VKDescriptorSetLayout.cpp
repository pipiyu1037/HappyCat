#include "VKDescriptorSetLayout.h"
#include "VKDevice.h"

namespace happycat {

std::unique_ptr<VKDescriptorSetLayout> VKDescriptorSetLayout::Create(
    VKDevice* device,
    const DescriptorSetLayoutDesc& desc)
{
    auto layout = std::unique_ptr<VKDescriptorSetLayout>(new VKDescriptorSetLayout());
    if (!layout->Initialize(device, desc)) {
        return nullptr;
    }
    return layout;
}

VKDescriptorSetLayout::~VKDescriptorSetLayout() {
    if (m_Layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
    }
}

bool VKDescriptorSetLayout::Initialize(VKDevice* device, const DescriptorSetLayoutDesc& desc) {
    m_Device = device->GetHandle();
    m_Desc = desc;

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(desc.bindings.size());

    for (const auto& binding : desc.bindings) {
        VkDescriptorSetLayoutBinding vkBinding{};
        vkBinding.binding = binding.binding;
        vkBinding.descriptorType = ToVkDescriptorType(binding.type);
        vkBinding.descriptorCount = binding.count;
        vkBinding.stageFlags = binding.stageFlags;
        vkBinding.pImmutableSamplers = binding.immutableSamplers;
        bindings.push_back(vkBinding);
    }

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.flags = desc.flags;
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &createInfo, nullptr, &m_Layout));

    return true;
}

} // namespace happycat
