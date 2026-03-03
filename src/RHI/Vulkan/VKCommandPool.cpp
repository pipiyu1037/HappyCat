#include "VKCommandPool.h"
#include "VKDevice.h"

namespace happycat {

std::unique_ptr<VKCommandPool> VKCommandPool::Create(VKDevice* device, u32 queueFamilyIndex) {
    auto pool = std::unique_ptr<VKCommandPool>(new VKCommandPool());
    if (!pool->Initialize(device, queueFamilyIndex)) {
        return nullptr;
    }
    return pool;
}

VKCommandPool::~VKCommandPool() {
    if (m_Pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_Device, m_Pool, nullptr);
    }
}

bool VKCommandPool::Initialize(VKDevice* device, u32 queueFamilyIndex) {
    m_Device = device->GetHandle();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex;

    VK_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_Pool));
    return true;
}

void VKCommandPool::Reset() {
    vkResetCommandPool(m_Device, m_Pool, 0);
}

void VKCommandPool::Trim() {
    vkTrimCommandPool(m_Device, m_Pool, 0);
}

} // namespace happycat
