#include "VKFramebuffer.h"
#include "VKDevice.h"
#include "VKRenderPass.h"

namespace happycat {

std::unique_ptr<VKFramebuffer> VKFramebuffer::Create(VKDevice* device, const CreateInfo& info) {
    auto framebuffer = std::unique_ptr<VKFramebuffer>(new VKFramebuffer());
    if (!framebuffer->Initialize(device, info)) {
        return nullptr;
    }
    return framebuffer;
}

VKFramebuffer::~VKFramebuffer() {
    if (m_Framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr);
    }
}

bool VKFramebuffer::Initialize(VKDevice* device, const CreateInfo& info) {
    m_Device = device->GetHandle();

    VkFramebufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = info.renderPass->GetHandle();
    createInfo.attachmentCount = static_cast<u32>(info.attachments.size());
    createInfo.pAttachments = info.attachments.data();
    createInfo.width = info.width;
    createInfo.height = info.height;
    createInfo.layers = info.layers;

    VK_CHECK(vkCreateFramebuffer(m_Device, &createInfo, nullptr, &m_Framebuffer));
    return true;
}

} // namespace happycat
