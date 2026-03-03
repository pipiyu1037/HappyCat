#include "VKCommandBuffer.h"
#include "VKDevice.h"
#include "VKCommandPool.h"

namespace happycat {

std::unique_ptr<VKCommandBuffer> VKCommandBuffer::Create(VKDevice* device, VKCommandPool* pool, Level level) {
    auto buffer = std::unique_ptr<VKCommandBuffer>(new VKCommandBuffer());
    if (!buffer->Initialize(device, pool, level)) {
        return nullptr;
    }
    return buffer;
}

VKCommandBuffer::~VKCommandBuffer() {
    // Command buffers are freed with the pool
}

bool VKCommandBuffer::Initialize(VKDevice* device, VKCommandPool* pool, Level level) {
    m_Device = device->GetHandle();
    m_Pool = pool->GetHandle();

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_Pool;
    allocInfo.level = level == Level::Primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;

    VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocInfo, &m_Buffer));
    return true;
}

void VKCommandBuffer::Begin(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;

    VK_CHECK(vkBeginCommandBuffer(m_Buffer, &beginInfo));
    m_IsRecording = true;
}

void VKCommandBuffer::End() {
    VK_CHECK(vkEndCommandBuffer(m_Buffer));
    m_IsRecording = false;
}

void VKCommandBuffer::Reset() {
    vkResetCommandBuffer(m_Buffer, 0);
    m_IsRecording = false;
}

void VKCommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo& beginInfo, VkSubpassContents contents) {
    vkCmdBeginRenderPass(m_Buffer, &beginInfo, contents);
}

void VKCommandBuffer::EndRenderPass() {
    vkCmdEndRenderPass(m_Buffer);
}

void VKCommandBuffer::BindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline) {
    vkCmdBindPipeline(m_Buffer, bindPoint, pipeline);
}

void VKCommandBuffer::SetViewport(u32 firstViewport, const std::vector<VkViewport>& viewports) {
    vkCmdSetViewport(m_Buffer, firstViewport, static_cast<u32>(viewports.size()), viewports.data());
}

void VKCommandBuffer::SetScissor(u32 firstScissor, const std::vector<VkRect2D>& scissors) {
    vkCmdSetScissor(m_Buffer, firstScissor, static_cast<u32>(scissors.size()), scissors.data());
}

void VKCommandBuffer::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) {
    vkCmdDraw(m_Buffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VKCommandBuffer::DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance) {
    vkCmdDrawIndexed(m_Buffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

} // namespace happycat
