#include "VKSemaphore.h"
#include "VKDevice.h"

namespace happycat {

std::unique_ptr<VKSemaphore> VKSemaphore::Create(VKDevice* device) {
    auto semaphore = std::unique_ptr<VKSemaphore>(new VKSemaphore());
    if (!semaphore->Initialize(device)) {
        return nullptr;
    }
    return semaphore;
}

VKSemaphore::~VKSemaphore() {
    if (m_Semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(m_Device, m_Semaphore, nullptr);
    }
}

bool VKSemaphore::Initialize(VKDevice* device) {
    m_Device = device->GetHandle();

    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_CHECK(vkCreateSemaphore(m_Device, &createInfo, nullptr, &m_Semaphore));
    return true;
}

} // namespace happycat
