#include "VKShaderModule.h"
#include "VKDevice.h"

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

VKShaderModule::~VKShaderModule() {
    if (m_Module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_Device, m_Module, nullptr);
    }
}

bool VKShaderModule::Initialize(VKDevice* device, const std::vector<char>& code) {
    m_Device = device->GetHandle();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const u32*>(code.data());

    VK_CHECK(vkCreateShaderModule(m_Device, &createInfo, nullptr, &m_Module));
    return true;
}

} // namespace happycat
