#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>

namespace happycat {

class VKInstance {
public:
    struct CreateInfo {
        std::string applicationName = "HappyCat Application";
        std::string engineName = "HappyCat";
        u32 applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        u32 engineVersion = VK_MAKE_VERSION(0, 1, 0);
        bool enableValidation = VULKAN_VALIDATION_ENABLED;
        Array<const char*> additionalExtensions;
    };

    static std::unique_ptr<VKInstance> Create(const CreateInfo& info);
    ~VKInstance();

    // Non-copyable
    VKInstance(const VKInstance&) = delete;
    VKInstance& operator=(const VKInstance&) = delete;

    // Getters
    VkInstance GetHandle() const { return m_Instance; }
    VkDebugUtilsMessengerEXT GetDebugMessenger() const { return m_DebugMessenger; }
    bool IsValidationEnabled() const { return m_ValidationEnabled; }

    // Get available physical devices
    const Array<VkPhysicalDevice>& GetPhysicalDevices() const { return m_PhysicalDevices; }

    // Pick best physical device
    VkPhysicalDevice PickPhysicalDevice(VkSurfaceKHR surface) const;

private:
    VKInstance() = default;
    bool Initialize(const CreateInfo& info);
    void SetupDebugMessenger();
    void DestroyDebugMessenger();

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    bool m_ValidationEnabled = false;

    Array<VkPhysicalDevice> m_PhysicalDevices;
    Array<const char*> m_EnabledExtensions;
    Array<const char*> m_EnabledLayers;
};

} // namespace happycat
