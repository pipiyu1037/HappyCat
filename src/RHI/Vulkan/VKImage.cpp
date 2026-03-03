#include "VKImage.h"
#include "VKDevice.h"

namespace happycat {

std::unique_ptr<VKImage> VKImage::Create(
    VKDevice* device,
    u32 width, u32 height,
    VkFormat format,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags)
{
    auto image = std::unique_ptr<VKImage>(new VKImage());
    if (!image->Initialize(device, width, height, format, usage, memoryFlags)) {
        return nullptr;
    }
    return image;
}

VKImage::~VKImage() {
    if (m_Image != VK_NULL_HANDLE) {
        vkDestroyImage(m_Device, m_Image, nullptr);
    }
    // Note: Memory should be freed with VMA or custom allocator
}

bool VKImage::Initialize(
    VKDevice* device,
    u32 width, u32 height,
    VkFormat format,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags)
{
    m_Device = device->GetHandle();
    m_Width = width;
    m_Height = height;

    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.extent.width = width;
    createInfo.extent.height = height;
    createInfo.extent.depth = 1;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.format = format;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateImage(m_Device, &createInfo, nullptr, &m_Image));

    // Allocate memory
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(m_Device, m_Image, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = device->FindMemoryType(memReqs.memoryTypeBits, memoryFlags);

    VK_CHECK(vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_Memory));

    // Bind memory
    VK_CHECK(vkBindImageMemory(m_Device, m_Image, m_Memory, 0));

    return true;
}

} // namespace happycat
