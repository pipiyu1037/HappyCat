#pragma once

#include "Core/Utils/Types.h"
#include "Core/Utils/Logger.h"
#include "Core/Utils/Assert.h"
#include "RHI/RHITypes.h"
#include "RHI/RHIDefs.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>
#include <set>

namespace happycat {

// Forward declarations
class VKDevice;
class VKPhysicalDevice;

// Vulkan utility functions
namespace VKUtils {

    // Check if format has depth aspect
    inline bool IsDepthFormat(VkFormat format) {
        switch (format) {
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return true;
            default:
                return false;
        }
    }

    inline bool IsDepthFormat(Format format) {
        return IsDepthFormat(ToVkFormat(format));
    }

    // Get format size in bytes
    inline u32 GetFormatSize(VkFormat format) {
        switch (format) {
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_SRGB:
                return 1;
            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_SRGB:
                return 2;
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SRGB:
                return 4;
            case VK_FORMAT_R16_UNORM:
            case VK_FORMAT_R16_SFLOAT:
                return 2;
            case VK_FORMAT_R16G16_UNORM:
            case VK_FORMAT_R16G16_SFLOAT:
                return 4;
            case VK_FORMAT_R16G16B16A16_UNORM:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return 8;
            case VK_FORMAT_R32_SFLOAT:
                return 4;
            case VK_FORMAT_R32G32_SFLOAT:
                return 8;
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return 16;
            case VK_FORMAT_D16_UNORM:
                return 2;
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return 4;
            case VK_FORMAT_D32_SFLOAT:
                return 4;
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return 5;
            default:
                return 0;
        }
    }

    // Get image aspect flags
    inline VkImageAspectFlags GetImageAspectFlags(VkFormat format, bool includeStencil = false) {
        if (IsDepthFormat(format)) {
            VkImageAspectFlags flags = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
                flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
                if (!includeStencil) {
                    flags = VK_IMAGE_ASPECT_DEPTH_BIT;
                }
            }
            return flags;
        }
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

    // Choose suitable depth format
    inline VkFormat ChooseDepthFormat(VkPhysicalDevice physicalDevice, bool prefer24Bit = true) {
        std::vector<VkFormat> candidates = {
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D16_UNORM
        };

        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                return format;
            }
        }

        return VK_FORMAT_D16_UNORM;
    }

    // Shader stage to Vulkan shader stage flag
    inline VkShaderStageFlagBits ToVkShaderStage(ShaderStage stage) {
        switch (stage) {
            case ShaderStage::Vertex:   return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
            case ShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
            case ShaderStage::Compute:  return VK_SHADER_STAGE_COMPUTE_BIT;
            default:                   return VK_SHADER_STAGE_VERTEX_BIT;
        }
    }

} // namespace VKUtils

} // namespace happycat
