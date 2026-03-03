#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

namespace happycat {

// Queue family indices
struct QueueFamilyIndices {
    std::optional<u32> graphics;
    std::optional<u32> compute;
    std::optional<u32> transfer;
    std::optional<u32> present;

    bool IsComplete() const {
        return graphics.has_value() && present.has_value();
    }

    bool HasCompute() const { return compute.has_value(); }
    bool HasTransfer() const { return transfer.has_value(); }
};

// Swap chain support details
struct SwapChainSupport {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    bool IsAdequate() const {
        return !formats.empty() && !presentModes.empty();
    }
};

class VKPhysicalDevice {
public:
    // Rate device suitability (higher is better)
    static i32 RateDevice(VkPhysicalDevice device, VkSurfaceKHR surface);

    static std::unique_ptr<VKPhysicalDevice> Create(VkPhysicalDevice device, VkSurfaceKHR surface);

    ~VKPhysicalDevice() = default;

    // Non-copyable
    VKPhysicalDevice(const VKPhysicalDevice&) = delete;
    VKPhysicalDevice& operator=(const VKPhysicalDevice&) = delete;

    // Getters
    VkPhysicalDevice GetHandle() const { return m_Device; }
    const VkPhysicalDeviceProperties& GetProperties() const { return m_Properties; }
    const VkPhysicalDeviceFeatures& GetFeatures() const { return m_Features; }
    const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_MemoryProperties; }
    const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilies; }
    const SwapChainSupport& GetSwapChainSupport() const { return m_SwapChainSupport; }

    // Find memory type
    u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const;

    // Check format support
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates,
                                  VkImageTiling tiling,
                                  VkFormatFeatureFlags features) const;

    // Get format properties
    void GetFormatProperties(VkFormat format, VkFormatProperties* outProperties) const {
        vkGetPhysicalDeviceFormatProperties(m_Device, format, outProperties);
    }

private:
    VKPhysicalDevice() = default;
    bool Initialize(VkPhysicalDevice device, VkSurfaceKHR surface);

    static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    static SwapChainSupport QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    VkPhysicalDevice m_Device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties m_Properties;
    VkPhysicalDeviceFeatures m_Features;
    VkPhysicalDeviceMemoryProperties m_MemoryProperties;
    QueueFamilyIndices m_QueueFamilies;
    SwapChainSupport m_SwapChainSupport;
};

} // namespace happycat
