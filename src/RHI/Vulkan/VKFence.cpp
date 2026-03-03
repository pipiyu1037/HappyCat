#include "VKFence.h"
#include "VKDevice.h"

namespace happycat {

std::unique_ptr<VKFence> VKFence::Create(VKDevice* device, bool signaled) {
    auto fence = std::unique_ptr<VKFence>(new VKFence());
    if (!fence->Initialize(device, signaled)) {
        return nullptr;
    }
    return fence;
}

VKFence::~VKFence() {
    if (m_Fence != VK_NULL_HANDLE) {
        vkDestroyFence(m_Device, m_Fence, nullptr);
    }
}

bool VKFence::Initialize(VKDevice* device, bool signaled) {
    m_Device = device->GetHandle();

    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VK_CHECK(vkCreateFence(m_Device, &createInfo, nullptr, &m_Fence));
    return true;
}

void VKFence::Wait(u64 timeout) {
    vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, timeout);
}

void VKFence::Reset() {
    vkResetFences(m_Device, 1, &m_Fence);
}

} // namespace happycat
