#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace happycat {

class VKDevice;

class VKCommandPool {
public:
    static std::unique_ptr<VKCommandPool> Create(VKDevice* device, u32 queueFamilyIndex);
    ~VKCommandPool();

    VKCommandPool(const VKCommandPool&) = delete;
    VKCommandPool& operator=(const VKCommandPool&) = delete;

    VkCommandPool GetHandle() const { return m_Pool; }

    void Reset();
    void Trim();

private:
    VKCommandPool() = default;
    bool Initialize(VKDevice* device, u32 queueFamilyIndex);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkCommandPool m_Pool = VK_NULL_HANDLE;
};

} // namespace happycat
