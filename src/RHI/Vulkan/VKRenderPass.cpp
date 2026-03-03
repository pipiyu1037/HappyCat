#include "VKRenderPass.h"
#include "VKDevice.h"

namespace happycat {

std::unique_ptr<VKRenderPass> VKRenderPass::Create(VKDevice* device, const CreateInfo& info) {
    auto renderPass = std::unique_ptr<VKRenderPass>(new VKRenderPass());
    if (!renderPass->Initialize(device, info)) {
        return nullptr;
    }
    return renderPass;
}

VKRenderPass::~VKRenderPass() {
    if (m_RenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    }
}

bool VKRenderPass::Initialize(VKDevice* device, const CreateInfo& info) {
    m_Device = device->GetHandle();

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorRefs;
    VkAttachmentReference depthRef{};

    u32 attachmentIndex = 0;

    // Color attachments
    for (const auto& color : info.colorAttachments) {
        VkAttachmentDescription desc{};
        desc.format = ToVkFormat(color.format);
        desc.samples = VK_SAMPLE_COUNT_1_BIT;
        desc.loadOp = color.loadOp;
        desc.storeOp = color.storeOp;
        desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout = color.initialLayout;
        desc.finalLayout = color.finalLayout;
        attachments.push_back(desc);

        VkAttachmentReference ref{};
        ref.attachment = attachmentIndex++;
        ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorRefs.push_back(ref);
    }

    // Depth attachment
    bool hasDepth = info.depthAttachment.has_value();
    if (hasDepth) {
        const auto& depth = info.depthAttachment.value();
        VkAttachmentDescription desc{};
        desc.format = ToVkFormat(depth.format);
        desc.samples = VK_SAMPLE_COUNT_1_BIT;
        desc.loadOp = depth.loadOp;
        desc.storeOp = depth.storeOp;
        desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout = depth.initialLayout;
        desc.finalLayout = depth.finalLayout;
        attachments.push_back(desc);

        depthRef.attachment = attachmentIndex;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    // Subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<u32>(colorRefs.size());
    subpass.pColorAttachments = colorRefs.data();
    subpass.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;

    // Subpass dependency
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    if (hasDepth) {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    // Create render pass
    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<u32>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(m_Device, &createInfo, nullptr, &m_RenderPass));
    return true;
}

} // namespace happycat
