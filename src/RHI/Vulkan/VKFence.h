#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace happycat {

class VKDevice;

class VKFence {
public:
    static std::unique_ptr<VKFence> Create(VKDevice* device, bool signaled = false);
    ~VKFence();

    VKFence(const VKFence&) = delete;
    VKFence& operator=(const VKFence&) = delete;

    VkFence GetHandle() const { return m_Fence; }

    void Wait(u64 timeout = UINT64_MAX);
    void Reset();

private:
    VKFence() = default;
    bool Initialize(VKDevice* device, bool signaled);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkFence m_Fence = VK_NULL_HANDLE;
};

} // namespace happycat
