#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace happycat {

class VKDevice;

class VKShaderModule {
public:
    static std::unique_ptr<VKShaderModule> Create(
        VKDevice* device,
        const std::vector<char>& code);

    ~VKShaderModule();

    VkShaderModule GetHandle() const { return m_Module; }

private:
    VKShaderModule() = default;
    bool Initialize(VKDevice* device, const std::vector<char>& code);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkShaderModule m_Module = VK_NULL_HANDLE;
};

} // namespace happycat
