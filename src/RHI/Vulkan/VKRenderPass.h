#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace happycat {

class VKDevice;

struct RenderPassAttachment {
    Format format = Format::RGBA8_SRGB;
    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};

class VKRenderPass {
public:
    struct CreateInfo {
        std::vector<RenderPassAttachment> colorAttachments;
        std::optional<RenderPassAttachment> depthAttachment;
    };

    static std::unique_ptr<VKRenderPass> Create(VKDevice* device, const CreateInfo& info);
    ~VKRenderPass();

    VkRenderPass GetHandle() const { return m_RenderPass; }

private:
    VKRenderPass() = default;
    bool Initialize(VKDevice* device, const CreateInfo& info);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
};

} // namespace happycat
