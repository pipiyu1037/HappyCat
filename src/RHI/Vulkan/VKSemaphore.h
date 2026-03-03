#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace happycat {

class VKDevice;

class VKSemaphore {
public:
    static std::unique_ptr<VKSemaphore> Create(VKDevice* device);
    ~VKSemaphore();

    VKSemaphore(const VKSemaphore&) = delete;
    VKSemaphore& operator=(const VKSemaphore&) = delete;

    VkSemaphore GetHandle() const { return m_Semaphore; }

private:
    VKSemaphore() = default;
    bool Initialize(VKDevice* device);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkSemaphore m_Semaphore = VK_NULL_HANDLE;
};

} // namespace happycat
