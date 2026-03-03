#include "VKPhysicalDevice.h"

namespace happycat {

i32 VKPhysicalDevice::RateDevice(VkPhysicalDevice device, VkSurfaceKHR surface) {
    // Get device properties
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    // Get device features
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    // Check queue families
    QueueFamilyIndices queueFamilies = FindQueueFamilies(device, surface);
    if (!queueFamilies.IsComplete()) {
        return 0;
    }

    // Check swap chain support
    SwapChainSupport swapChainSupport = QuerySwapChainSupport(device, surface);
    if (!swapChainSupport.IsAdequate()) {
        return 0;
    }

    // Check required extensions
    u32 extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(std::begin(DEVICE_EXTENSIONS), std::end(DEVICE_EXTENSIONS));
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    if (!requiredExtensions.empty()) {
        return 0;
    }

    // Score the device
    i32 score = 1;

    // Discrete GPUs have a significant performance advantage
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += static_cast<i32>(props.limits.maxImageDimension2D);

    // Required features
    if (!features.samplerAnisotropy) {
        score = 0;
    }

    return score;
}

std::unique_ptr<VKPhysicalDevice> VKPhysicalDevice::Create(VkPhysicalDevice device, VkSurfaceKHR surface) {
    auto physicalDevice = std::unique_ptr<VKPhysicalDevice>(new VKPhysicalDevice());
    if (!physicalDevice->Initialize(device, surface)) {
        HC_CORE_ERROR("Failed to initialize physical device");
        return nullptr;
    }
    return physicalDevice;
}

bool VKPhysicalDevice::Initialize(VkPhysicalDevice device, VkSurfaceKHR surface) {
    m_Device = device;

    // Get properties
    vkGetPhysicalDeviceProperties(m_Device, &m_Properties);
    HC_CORE_INFO("GPU: {0}", m_Properties.deviceName);

    // Get features
    vkGetPhysicalDeviceFeatures(m_Device, &m_Features);

    // Get memory properties
    vkGetPhysicalDeviceMemoryProperties(m_Device, &m_MemoryProperties);

    // Find queue families
    m_QueueFamilies = FindQueueFamilies(m_Device, surface);

    // Query swap chain support
    m_SwapChainSupport = QuerySwapChainSupport(m_Device, surface);

    return true;
}

QueueFamilyIndices VKPhysicalDevice::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    u32 i = 0;
    for (const auto& queueFamily : queueFamilies) {
        // Graphics queue
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics = i;
        }

        // Compute queue (prefer dedicated)
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            if (!indices.compute.has_value() || !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                indices.compute = i;
            }
        }

        // Transfer queue (prefer dedicated)
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            if (!indices.transfer.has_value() ||
                !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) ||
                !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                indices.transfer = i;
            }
        }

        // Present support
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.present = i;
        }

        if (indices.IsComplete()) {
            if (indices.graphics == indices.present) {
                break;  // Prefer same queue for graphics and present
            }
        }

        i++;
    }

    return indices;
}

SwapChainSupport VKPhysicalDevice::QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupport details;

    // Capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // Formats
    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // Present modes
    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

u32 VKPhysicalDevice::FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const {
    for (u32 i = 0; i < m_MemoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (m_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    HC_CORE_ERROR("Failed to find suitable memory type");
    return 0;
}

VkFormat VKPhysicalDevice::FindSupportedFormat(
    const std::vector<VkFormat>& candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_Device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    HC_CORE_ERROR("Failed to find supported format");
    return VK_FORMAT_UNDEFINED;
}

} // namespace happycat
