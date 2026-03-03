#pragma once

#include "Core/Utils/Types.h"

// Platform-specific macros
#if defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

namespace happycat {

// Vulkan version
constexpr u32 VULKAN_API_VERSION = VK_API_VERSION_1_3;

// Max frames in flight
constexpr u32 MAX_FRAMES_IN_FLIGHT = 3;

// Max command buffers per frame
constexpr u32 MAX_COMMAND_BUFFERS_PER_FRAME = 16;

// Max descriptor sets
constexpr u32 MAX_DESCRIPTOR_SETS = 1024;

// Max push constant size
constexpr u32 MAX_PUSH_CONSTANT_SIZE = 128;

// Default viewport
constexpr u32 DEFAULT_WINDOW_WIDTH = 1280;
constexpr u32 DEFAULT_WINDOW_HEIGHT = 720;

// Validation layers
#ifdef HC_VULKAN_VALIDATION
    constexpr bool VULKAN_VALIDATION_ENABLED = true;
#else
    constexpr bool VULKAN_VALIDATION_ENABLED = false;
#endif

inline const char* VALIDATION_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation"
};

inline const char* INSTANCE_EXTENSIONS[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
    "VK_KHR_win32_surface",
#elif defined(__linux__)
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
#ifdef HC_VULKAN_VALIDATION
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};

inline const char* DEVICE_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
};

// Vulkan result checking
#define VK_CHECK(call) \
    do { \
        VkResult result = call; \
        if (result != VK_SUCCESS) { \
            HC_CORE_ERROR("Vulkan error: {0} at {1}:{2}", \
                static_cast<u32>(result), __FILE__, __LINE__); \
        } \
    } while (false)

#define VK_CHECK_RESULT(call, errorMsg) \
    do { \
        VkResult result = call; \
        if (result != VK_SUCCESS) { \
            HC_CORE_ERROR("Vulkan error: {0} - {1}", \
                static_cast<u32>(result), errorMsg); \
            return false; \
        } \
    } while (false)

} // namespace happycat
