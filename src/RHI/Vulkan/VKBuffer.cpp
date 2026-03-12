#include "VKBuffer.h"
#include "VKDevice.h"

namespace happycat {

std::unique_ptr<VKBuffer> VKBuffer::Create(
    VKDevice* device,
    u64 size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags)
{
    auto buffer = std::unique_ptr<VKBuffer>(new VKBuffer());
    if (!buffer->Initialize(device, size, usage, memoryFlags)) {
        return nullptr;
    }
    return buffer;
}

VKBuffer::~VKBuffer() {
    Unmap();

    if (m_Buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_Device, m_Buffer, nullptr);
    }

    if (m_Memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_Device, m_Memory, nullptr);
    }
}

bool VKBuffer::Initialize(
    VKDevice* device,
    u64 size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags)
{
    m_Device = device->GetHandle();
    m_Size = size;

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateBuffer(m_Device, &createInfo, nullptr, &m_Buffer));

    // Allocate memory
    VkMemoryRequirements memReqs{};
    vkGetBufferMemoryRequirements(m_Device, m_Buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = device->FindMemoryType(memReqs.memoryTypeBits, memoryFlags);

    VK_CHECK(vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_Memory));

    // Bind memory
    VK_CHECK(vkBindBufferMemory(m_Device, m_Buffer, m_Memory, 0));

    return true;
}

void* VKBuffer::Map() {
    if (m_MappedData == nullptr) {
        VK_CHECK(vkMapMemory(m_Device, m_Memory, 0, m_Size, 0, &m_MappedData));
    }
    return m_MappedData;
}

void VKBuffer::Unmap() {
    if (m_MappedData != nullptr) {
        vkUnmapMemory(m_Device, m_Memory);
        m_MappedData = nullptr;
    }
}

} // namespace happycat
