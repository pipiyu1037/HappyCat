#include "VKSampler.h"
#include "VKDevice.h"

namespace happycat {

std::unique_ptr<VKSampler> VKSampler::Create(VKDevice* device, const SamplerDesc& desc) {
    auto sampler = std::unique_ptr<VKSampler>(new VKSampler());
    if (!sampler->Initialize(device, desc)) {
        return nullptr;
    }
    return sampler;
}

VKSampler::~VKSampler() {
    if (m_Sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_Device, m_Sampler, nullptr);
    }
}

bool VKSampler::Initialize(VKDevice* device, const SamplerDesc& desc) {
    m_Device = device->GetHandle();

    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = desc.magFilter;
    createInfo.minFilter = desc.minFilter;
    createInfo.mipmapMode = desc.mipmapMode;
    createInfo.addressModeU = desc.addressModeU;
    createInfo.addressModeV = desc.addressModeV;
    createInfo.addressModeW = desc.addressModeW;
    createInfo.mipLodBias = desc.mipLodBias;
    createInfo.anisotropyEnable = desc.anisotropyEnable ? VK_TRUE : VK_FALSE;
    createInfo.maxAnisotropy = desc.maxAnisotropy;
    createInfo.compareEnable = desc.compareEnable ? VK_TRUE : VK_FALSE;
    createInfo.compareOp = desc.compareOp;
    createInfo.minLod = desc.minLod;
    createInfo.maxLod = desc.maxLod;
    createInfo.borderColor = desc.borderColor;
    createInfo.unnormalizedCoordinates = desc.unnormalizedCoordinates;

    VK_CHECK(vkCreateSampler(m_Device, &createInfo, nullptr, &m_Sampler));

    return true;
}

} // namespace happycat
