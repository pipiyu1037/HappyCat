#include "VKSwapChain.h"
#include "VKDevice.h"
#include "VKPhysicalDevice.h"
#include <algorithm>

namespace happycat {

std::unique_ptr<VKSwapChain> VKSwapChain::Create(VKDevice* device, const CreateInfo& info) {
    auto swapChain = std::unique_ptr<VKSwapChain>(new VKSwapChain());
    if (!swapChain->Initialize(device, info)) {
        HC_CORE_ERROR("Failed to create swap chain");
        return nullptr;
    }
    return swapChain;
}

VKSwapChain::~VKSwapChain() {
    Cleanup();
}

bool VKSwapChain::Initialize(VKDevice* device, const CreateInfo& info) {
    m_Device = device;
    m_Surface = info.surface;
    m_Vsync = info.vsync;

    auto* physicalDevice = device->GetPhysicalDevice();
    const auto& support = physicalDevice->GetSwapChainSupport();

    // Choose surface format
    VkSurfaceFormatKHR surfaceFormat = support.formats[0];
    for (const auto& format : support.formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = format;
            break;
        }
    }
    m_ImageFormat = surfaceFormat.format;

    // Choose present mode
    m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;  // Guaranteed available
    if (!m_Vsync) {
        for (const auto& mode : support.presentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                m_PresentMode = mode;
                break;
            } else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                m_PresentMode = mode;
            }
        }
    }

    // Choose extent
    m_Extent = support.capabilities.currentExtent;
    if (m_Extent.width == UINT32_MAX) {
        m_Extent.width = std::clamp(info.width,
            support.capabilities.minImageExtent.width,
            support.capabilities.maxImageExtent.width);
        m_Extent.height = std::clamp(info.height,
            support.capabilities.minImageExtent.height,
            support.capabilities.maxImageExtent.height);
    }

    // Image count
    u32 imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, support.capabilities.maxImageCount);
    }

    // Create swapchain
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_Extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = m_PresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    const auto& indices = physicalDevice->GetQueueFamilyIndices();
    u32 queueFamilyIndices[] = { indices.graphics.value(), indices.present.value() };

    if (indices.graphics != indices.present) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VK_CHECK(vkCreateSwapchainKHR(device->GetHandle(), &createInfo, nullptr, &m_SwapChain));

    // Get images
    vkGetSwapchainImagesKHR(device->GetHandle(), m_SwapChain, &imageCount, nullptr);
    m_Images.resize(imageCount);
    vkGetSwapchainImagesKHR(device->GetHandle(), m_SwapChain, &imageCount, m_Images.data());

    CreateImageViews();

    HC_CORE_INFO("Swap chain created: {0}x{1}, {2} images",
        m_Extent.width, m_Extent.height, imageCount);

    return true;
}

void VKSwapChain::CreateImageViews() {
    m_ImageViews.resize(m_Images.size());

    for (size_t i = 0; i < m_Images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_Images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_ImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(m_Device->GetHandle(), &createInfo, nullptr, &m_ImageViews[i]));
    }
}

void VKSwapChain::Cleanup() {
    for (auto& view : m_ImageViews) {
        vkDestroyImageView(m_Device->GetHandle(), view, nullptr);
    }
    m_ImageViews.clear();
    m_Images.clear();

    if (m_SwapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_Device->GetHandle(), m_SwapChain, nullptr);
        m_SwapChain = VK_NULL_HANDLE;
    }
}

u32 VKSwapChain::AcquireNextImage(VkSemaphore semaphore, VkFence fence) {
    u32 imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        m_Device->GetHandle(),
        m_SwapChain,
        UINT64_MAX,
        semaphore,
        fence,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return UINT32_MAX;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VK_CHECK(result);
    }

    return imageIndex;
}

void VKSwapChain::Present(VkQueue queue, u32 imageIndex, VkSemaphore waitSemaphore) {
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_SwapChain;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(queue, &presentInfo);
}

void VKSwapChain::Recreate(u32 width, u32 height) {
    // Don't recreate with zero size (minimized window)
    if (width == 0 || height == 0) {
        HC_CORE_WARN("SwapChain::Recreate called with zero size, skipping");
        return;
    }

    m_Device->WaitIdle();

    Cleanup();

    CreateInfo info{};
    info.width = width;
    info.height = height;
    info.vsync = m_Vsync;
    info.surface = m_Surface;

    Initialize(m_Device, info);
}

} // namespace happycat
