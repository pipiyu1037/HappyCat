#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace happycat {

class VKDevice;
class VKRenderPass;
class VKImageView;

class VKFramebuffer {
public:
    struct CreateInfo {
        VKRenderPass* renderPass = nullptr;
        std::vector<VkImageView> attachments;
        u32 width = 1;
        u32 height = 1;
        u32 layers = 1;
    };

    static std::unique_ptr<VKFramebuffer> Create(VKDevice* device, const CreateInfo& info);
    ~VKFramebuffer();

    VkFramebuffer GetHandle() const { return m_Framebuffer; }

private:
    VKFramebuffer() = default;
    bool Initialize(VKDevice* device, const CreateInfo& info);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;
};

} // namespace happycat
