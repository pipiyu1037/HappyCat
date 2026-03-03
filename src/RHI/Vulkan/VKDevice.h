#pragma once

#include "VKCommon.h"
#include "VKPhysicalDevice.h"
#include "VKQueue.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <unordered_map>

namespace happycat {

class VKDevice {
public:
    struct CreateInfo {
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        bool enableValidation = VULKAN_VALIDATION_ENABLED;
    };

    static std::unique_ptr<VKDevice> Create(const CreateInfo& info);
    ~VKDevice();

    // Non-copyable
    VKDevice(const VKDevice&) = delete;
    VKDevice& operator=(const VKDevice&) = delete;

    // Getters
    VkDevice GetHandle() const { return m_Device; }
    VkPhysicalDevice GetPhysicalDeviceHandle() const { return m_PhysicalDevice->GetHandle(); }
    VKPhysicalDevice* GetPhysicalDevice() const { return m_PhysicalDevice.get(); }

    VKQueue* GetGraphicsQueue() const { return m_GraphicsQueue.get(); }
    VKQueue* GetComputeQueue() const { return m_ComputeQueue.get(); }
    VKQueue* GetTransferQueue() const { return m_TransferQueue.get(); }
    VKQueue* GetPresentQueue() const { return m_PresentQueue.get(); }

    // Commands
    void WaitIdle();

    // Memory allocation helper
    u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const {
        return m_PhysicalDevice->FindMemoryType(typeFilter, properties);
    }

private:
    VKDevice() = default;
    bool Initialize(const CreateInfo& info);

    VkDevice m_Device = VK_NULL_HANDLE;
    std::unique_ptr<VKPhysicalDevice> m_PhysicalDevice;

    std::unique_ptr<VKQueue> m_GraphicsQueue;
    std::unique_ptr<VKQueue> m_ComputeQueue;
    std::unique_ptr<VKQueue> m_TransferQueue;
    std::unique_ptr<VKQueue> m_PresentQueue;
};

} // namespace happycat
