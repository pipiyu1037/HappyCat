#include "VKShaderModule.h"
#include "VKDevice.h"
#include "Core/Utils/Logger.h"

#include <cstring>

namespace happycat {

std::unique_ptr<VKShaderModule> VKShaderModule::Create(
    VKDevice* device,
    const std::vector<char>& code)
{
    auto module = std::unique_ptr<VKShaderModule>(new VKShaderModule());
    if (!module->Initialize(device, code)) {
        return nullptr;
    }
    return module;
}

std::unique_ptr<VKShaderModule> VKShaderModule::Create(
    VKDevice* device,
    const std::vector<u32>& spirv)
{
    if (spirv.empty()) {
        HC_CORE_ERROR("SPIR-V code is empty");
        return nullptr;
    }
    // Convert u32 vector to char vector
    std::vector<char> code(spirv.size() * sizeof(u32));
    std::memcpy(code.data(), spirv.data(), code.size());
    return Create(device, code);
}

VKShaderModule::~VKShaderModule() {
    if (m_Module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_Device, m_Module, nullptr);
    }
}

bool VKShaderModule::Initialize(VKDevice* device, const std::vector<char>& code) {
    if (code.empty()) {
        return false;
    }

    m_Device = device->GetHandle();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const u32*>(code.data());

    if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &m_Module) != VK_SUCCESS) {
        HC_CORE_ERROR("Failed to create Vulkan shader module");
        return false;
    }
    return true;
}

} // namespace happycat
