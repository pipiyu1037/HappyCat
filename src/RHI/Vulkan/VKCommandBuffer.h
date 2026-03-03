#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace happycat {

class VKDevice;
class VKCommandPool;

class VKCommandBuffer {
public:
    enum class Level { Primary, Secondary };

    static std::unique_ptr<VKCommandBuffer> Create(VKDevice* device, VKCommandPool* pool, Level level = Level::Primary);
    ~VKCommandBuffer();

    VKCommandBuffer(const VKCommandBuffer&) = delete;
    VKCommandBuffer& operator=(const VKCommandBuffer&) = delete;

    VkCommandBuffer GetHandle() const { return m_Buffer; }

    void Begin(VkCommandBufferUsageFlags flags = 0);
    void End();
    void Reset();

    // Drawing commands
    void BeginRenderPass(const VkRenderPassBeginInfo& beginInfo, VkSubpassContents contents);
    void EndRenderPass();
    void BindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline);
    void SetViewport(u32 firstViewport, const std::vector<VkViewport>& viewports);
    void SetScissor(u32 firstScissor, const std::vector<VkRect2D>& scissors);
    void Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance);
    void DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance);

private:
    VKCommandBuffer() = default;
    bool Initialize(VKDevice* device, VKCommandPool* pool, Level level);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkCommandPool m_Pool = VK_NULL_HANDLE;
    VkCommandBuffer m_Buffer = VK_NULL_HANDLE;
    bool m_IsRecording = false;
};

} // namespace happycat
