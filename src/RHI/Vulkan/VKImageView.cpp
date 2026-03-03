#include "VKImageView.h"
#include "VKDevice.h"

namespace happycat {

std::unique_ptr<VKImageView> VKImageView::Create(
    VKDevice* device,
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags)
{
    auto view = std::unique_ptr<VKImageView>(new VKImageView());
    if (!view->Initialize(device, image, format, aspectFlags)) {
        return nullptr;
    }
    return view;
}

VKImageView::~VKImageView() {
    if (m_ImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_Device, m_ImageView, nullptr);
    }
}

bool VKImageView::Initialize(
    VKDevice* device,
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags)
{
    m_Device = device->GetHandle();

    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_Device, &createInfo, nullptr, &m_ImageView));
    return true;
}

} // namespace happycat
