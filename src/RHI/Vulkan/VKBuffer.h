#pragma once

#include "VKCommon.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace happycat {

class VKDevice;

class VKBuffer {
public:
    static std::unique_ptr<VKBuffer> Create(
        VKDevice* device,
        u64 size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags memoryFlags);

    ~VKBuffer();

    VkBuffer GetHandle() const { return m_Buffer; }
    VkDeviceMemory GetMemory() const { return m_Memory; }
    u64 GetSize() const { return m_Size; }
    void* Map();
    void Unmap();

private:
    VKBuffer() = default;
    bool Initialize(
        VKDevice* device,
        u64 size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags memoryFlags);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory{};
    u64 m_Size = 0;
    void* m_MappedData = nullptr;
};

} // namespace happycat
