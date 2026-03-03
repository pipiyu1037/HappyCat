#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace happycat {

class VKDevice;

class VKImage {
public:
    static std::unique_ptr<VKImage> Create(
        VKDevice* device,
        u32 width, u32 height,
        VkFormat format,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryFlags);

    ~VKImage();

    VkImage GetHandle() const { return m_Image; }
    VkDeviceMemory GetMemory() const { return m_Memory; }
    u32 GetWidth() const { return m_Width; }
    u32 GetHeight() const { return m_Height; }

private:
    VKImage() = default;
    bool Initialize(
        VKDevice* device,
        u32 width, u32 height,
        VkFormat format,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryFlags);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory{};
    u32 m_Width = 0;
    u32 m_Height = 1;
};

} // namespace happycat
