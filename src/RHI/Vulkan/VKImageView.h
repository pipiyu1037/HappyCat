#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace happycat {

class VKDevice;
class VKImage;

class VKImageView {
public:
    static std::unique_ptr<VKImageView> Create(
        VKDevice* device,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags);

    ~VKImageView();

    VkImageView GetHandle() const { return m_ImageView; }

private:
    VKImageView() = default;
    bool Initialize(
        VKDevice* device,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
};

} // namespace happycat
