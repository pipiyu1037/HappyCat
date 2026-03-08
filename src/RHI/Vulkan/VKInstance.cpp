#include "VKInstance.h"
#include "VKPhysicalDevice.h"

#include <algorithm>

namespace happycat {

// Debug callback
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            HC_CORE_TRACE(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            HC_CORE_INFO(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            HC_CORE_WARN(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            HC_CORE_ERROR(pCallbackData->pMessage);
            break;
        default:
            HC_CORE_INFO(pCallbackData->pMessage);
            break;
    }
    return VK_FALSE;
}

std::unique_ptr<VKInstance> VKInstance::Create(const CreateInfo& info) {
    auto instance = std::unique_ptr<VKInstance>(new VKInstance());
    if (!instance->Initialize(info)) {
        HC_CORE_ERROR("Failed to create Vulkan instance");
        return nullptr;
    }
    return instance;
}

VKInstance::~VKInstance() {
    if (m_DebugMessenger != VK_NULL_HANDLE) {
        DestroyDebugMessenger();
    }

    if (m_Instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_Instance, nullptr);
        HC_CORE_INFO("Vulkan instance destroyed");
    }
}

bool VKInstance::Initialize(const CreateInfo& info) {
    m_ValidationEnabled = info.enableValidation;

    // Application info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = info.applicationName.c_str();
    appInfo.applicationVersion = info.applicationVersion;
    appInfo.pEngineName = info.engineName.c_str();
    appInfo.engineVersion = info.engineVersion;
    appInfo.apiVersion = VULKAN_API_VERSION;

    // Get available extensions
    u32 extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    HC_CORE_INFO("Available Vulkan extensions: {0}", extensionCount);

    // Required extensions
    std::vector<const char*> requiredExtensions;

    // Add surface extensions (required for GLFW)
    requiredExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
    requiredExtensions.push_back("VK_KHR_win32_surface");
#else
    requiredExtensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
    requiredExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif

    // Debug utils extension for validation
    if (m_ValidationEnabled) {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Add additional extensions
    for (const char* ext : info.additionalExtensions) {
        requiredExtensions.push_back(ext);
    }

    // Check extension availability
    for (const char* required : requiredExtensions) {
        bool found = false;
        for (const auto& available : availableExtensions) {
            if (strcmp(required, available.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            HC_CORE_ERROR("Required Vulkan extension not available: {0}", required);
            return false;
        }
        m_EnabledExtensions.push_back(required);
    }

    // Instance create info
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<u32>(m_EnabledExtensions.size());
    createInfo.ppEnabledExtensionNames = m_EnabledExtensions.data();

    // Validation layers
    if (m_ValidationEnabled) {
        // Check layer availability
        u32 layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // Need to check
        bool layerFound = false;
        for (const char* layerName : VALIDATION_LAYERS) {
            for (const auto& layerProps : availableLayers) {
                if (strcmp(layerName, layerProps.layerName) == 0) {
                    m_EnabledLayers.push_back(layerName);
                    layerFound = true;
                    break;
                }
            }
        }

        if (layerFound) {
            createInfo.enabledLayerCount = static_cast<u32>(m_EnabledLayers.size());
            createInfo.ppEnabledLayerNames = m_EnabledLayers.data();
            HC_CORE_INFO("Vulkan validation layers enabled");
        } else {
            HC_CORE_WARN("Vulkan validation layers not available");
            m_ValidationEnabled = false;
        }
    }

    // Create instance
    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
    if (result != VK_SUCCESS) {
        HC_CORE_ERROR("Failed to create Vulkan instance: {0}", static_cast<i32>(result));
        return false;
    }

    HC_CORE_INFO("Vulkan instance created successfully");

    // Setup debug messenger
    if (m_ValidationEnabled) {
        SetupDebugMessenger();
    }

    // Enumerate physical devices
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        HC_CORE_ERROR("No Vulkan-capable GPUs found");
        return false;
    }

    m_PhysicalDevices.resize(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, m_PhysicalDevices.data());

    HC_CORE_INFO("Found {0} Vulkan-capable GPU(s)", deviceCount);

    return true;
}

void VKInstance::SetupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_Instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) {
        VkResult result = func(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
        if (result == VK_SUCCESS) {
            HC_CORE_INFO("Vulkan debug messenger created");
        }
    } else {
        HC_CORE_WARN("Failed to load vkCreateDebugUtilsMessengerEXT");
    }
}

void VKInstance::DestroyDebugMessenger() {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_Instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr) {
        func(m_Instance, m_DebugMessenger, nullptr);
    }}

VkPhysicalDevice VKInstance::PickPhysicalDevice(VkSurfaceKHR surface) const {
    // Score each device and pick the best one
    i32 bestScore = -1;
    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

    for (VkPhysicalDevice device : m_PhysicalDevices) {
        i32 score = VKPhysicalDevice::RateDevice(device, surface);
        if (score > bestScore) {
            bestScore = score;
            bestDevice = device;
        }
    }

    if (bestDevice == VK_NULL_HANDLE) {
        HC_CORE_ERROR("No suitable GPU found");
    }

    return bestDevice;
}

} // namespace happycat
