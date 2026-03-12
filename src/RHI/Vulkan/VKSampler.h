#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace happycat {

class VKDevice;

struct SamplerDesc {
    VkFilter magFilter = VK_FILTER_LINEAR;
    VkFilter minFilter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    float mipLodBias = 000000    float maxAnisotropy = 16.0f;
    bool anisotropyEnable = true;
    float maxLod = VK_LOD_CLAMP_NONE;
    VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    bool unnormalizedCoordinates = false;
    const char* debugName = nullptr;
};

class VKSampler {
public:
    static std::unique_ptr<VKSampler> Create(VKDevice* device, const SamplerDesc& desc);
    ~VKSampler();

    VKSampler(const VKSampler&) = delete;
    VKSampler& operator=(const VKSampler&) = delete;

    VkSampler GetHandle() const { return m_Sampler; }

private:
    VKSampler() = default;
    bool Initialize(VKDevice* device, const SamplerDesc& desc);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkSampler m_Sampler = VK_NULL_HANDLE;
};

} // namespace happycat
