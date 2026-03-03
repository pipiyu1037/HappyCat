#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace happycat {

class VKDevice;
class VKPhysicalDevice;

class VKSwapChain {
public:
    struct CreateInfo {
        u32 width = 1280;
        u32 height = 720;
        bool vsync = true;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
    };

    static std::unique_ptr<VKSwapChain> Create(VKDevice* device, const CreateInfo& info);
    ~VKSwapChain();

    VKSwapChain(const VKSwapChain&) = delete;
    VKSwapChain& operator=(const VKSwapChain&) = delete;

    VkSwapchainKHR GetHandle() const { return m_SwapChain; }
    VkFormat GetImageFormat() const { return m_ImageFormat; }
    VkExtent2D GetExtent() const { return m_Extent; }
    const std::vector<VkImage>& GetImages() const { return m_Images; }
    const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }
    u32 GetImageCount() const { return static_cast<u32>(m_Images.size()); }

    // Acquire next image
    u32 AcquireNextImage(VkSemaphore semaphore, VkFence fence = VK_NULL_HANDLE);

    // Present
    void Present(VkQueue queue, u32 imageIndex, VkSemaphore waitSemaphore);

    // Recreate swapchain (on resize)
    void Recreate(u32 width, u32 height);

private:
    VKSwapChain() = default;
    bool Initialize(VKDevice* device, const CreateInfo& info);
    void CreateImageViews();
    void Cleanup();

    VKDevice* m_Device = nullptr;
    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;

    VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_Extent = {};
    VkPresentModeKHR m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;

    bool m_Vsync = true;
};

} // namespace happycat
