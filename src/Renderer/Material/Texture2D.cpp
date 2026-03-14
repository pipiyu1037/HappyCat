#include "Texture2D.h"
#include "RHI/Vulkan/VKDevice.h"
#include "RHI/Vulkan/VKImage.h"
#include "RHI/Vulkan/VKImageView.h"
#include "RHI/Vulkan/VKSampler.h"
#include "RHI/Vulkan/VKBuffer.h"
#include "RHI/Vulkan/VKQueue.h"
#include "Core/Utils/Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>

namespace happycat {

Scope<Texture2D> Texture2D::Create(VKDevice* device, const Texture2DDesc& desc) {
    auto texture = Scope<Texture2D>(new Texture2D());
    if (!texture->Initialize(device, desc)) {
        HC_CORE_ERROR("Failed to create texture from file: {0}", desc.filePath);
        return nullptr;
    }
    return texture;
}

Scope<Texture2D> Texture2D::CreateFromMemory(
    VKDevice* device,
    const u8* data,
    u32 width,
    u32 height,
    u32 channels,
    const Texture2DDesc& desc)
{
    auto texture = Scope<Texture2D>(new Texture2D());

    // Store dimensions
    texture->m_Device = device;
    texture->m_Width = width;
    texture->m_Height = height;
    texture->m_Format = desc.format;
    texture->m_FilePath = desc.debugName ? desc.debugName : "";

    // Calculate mip levels
    if (desc.generateMipmaps) {
        texture->m_MipLevels = static_cast<u32>(std::floor(std::log2(std::max(width, height)))) + 1;
    } else {
        texture->m_MipLevels = 1;
    }

    // Upload pixel data
    if (!texture->UploadData(device, data, width, height, channels)) {
        HC_CORE_ERROR("Failed to upload texture data from memory");
        return nullptr;
    }

    // Create image view
    texture->m_ImageView = VKImageView::Create(
        device,
        texture->m_Image->GetHandle(),
        desc.format,
        VK_IMAGE_ASPECT_COLOR_BIT);

    if (!texture->m_ImageView) {
        HC_CORE_ERROR("Failed to create image view");
        return nullptr;
    }

    // Create sampler
    SamplerDesc samplerDesc{};
    samplerDesc.magFilter = desc.magFilter;
    samplerDesc.minFilter = desc.minFilter;
    samplerDesc.addressModeU = desc.addressModeU;
    samplerDesc.addressModeV = desc.addressModeV;
    samplerDesc.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerDesc.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerDesc.maxLod = static_cast<float>(texture->m_MipLevels);
    samplerDesc.anisotropyEnable = true;
    samplerDesc.maxAnisotropy = 16.0f;
    samplerDesc.debugName = desc.debugName;

    texture->m_Sampler = VKSampler::Create(device, samplerDesc);
    if (!texture->m_Sampler) {
        HC_CORE_ERROR("Failed to create sampler");
        return nullptr;
    }

    return texture;
}

Texture2D::~Texture2D() {
    // Resources are automatically cleaned up by smart pointers
}

bool Texture2D::Initialize(VKDevice* device, const Texture2DDesc& desc) {
    m_Device = device;
    m_Format = desc.format;
    m_FilePath = desc.filePath;

    // Load image using stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);

    u8* pixels = stbi_load(desc.filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        HC_CORE_ERROR("Failed to load texture: {0}", desc.filePath);
        return false;
    }

    m_Width = static_cast<u32>(width);
    m_Height = static_cast<u32>(height);

    // Calculate mip levels
    if (desc.generateMipmaps) {
        m_MipLevels = static_cast<u32>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1;
    } else {
        m_MipLevels = 1;
    }

    // Upload pixel data
    bool success = UploadData(device, pixels, m_Width, m_Height, 4); // STBI_rgb_alpha = 4 channels
    stbi_image_free(pixels);

    if (!success) {
        return false;
    }

    // Create image view
    m_ImageView = VKImageView::Create(
        device,
        m_Image->GetHandle(),
        desc.format,
        VK_IMAGE_ASPECT_COLOR_BIT);

    if (!m_ImageView) {
        HC_CORE_ERROR("Failed to create image view for texture: {0}", desc.filePath);
        return false;
    }

    // Create sampler
    SamplerDesc samplerDesc{};
    samplerDesc.magFilter = desc.magFilter;
    samplerDesc.minFilter = desc.minFilter;
    samplerDesc.addressModeU = desc.addressModeU;
    samplerDesc.addressModeV = desc.addressModeV;
    samplerDesc.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerDesc.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerDesc.maxLod = static_cast<float>(m_MipLevels);
    samplerDesc.anisotropyEnable = true;
    samplerDesc.maxAnisotropy = 16.0f;
    samplerDesc.debugName = desc.debugName;

    m_Sampler = VKSampler::Create(device, samplerDesc);
    if (!m_Sampler) {
        HC_CORE_ERROR("Failed to create sampler for texture: {0}", desc.filePath);
        return false;
    }

    HC_CORE_INFO("Created texture: {0} ({1}x{2}, {3} mips)",
        desc.filePath, m_Width, m_Height, m_MipLevels);
    return true;
}

bool Texture2D::UploadData(VKDevice* device, const u8* pixelData, u32 width, u32 height, u32 channels) {
    VkDeviceSize imageSize = width * height * channels;

    // Create staging buffer
    auto stagingBuffer = VKBuffer::Create(
        device,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (!stagingBuffer) {
        HC_CORE_ERROR("Failed to create staging buffer for texture upload");
        return false;
    }

    // Copy pixel data to staging buffer
    void* data = stagingBuffer->Map();
    memcpy(data, pixelData, static_cast<size_t>(imageSize));
    stagingBuffer->Unmap();

    // Create image with transfer destination and sampling support
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                              VK_IMAGE_USAGE_SAMPLED_BIT;

    m_Image = VKImage::Create(
        device,
        width, height,
        m_Format,
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (!m_Image) {
        HC_CORE_ERROR("Failed to create image for texture");
        return false;
    }

    // Transition to transfer destination layout
    TransitionImageLayout(
        device,
        m_Image->GetHandle(),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        m_MipLevels);

    // Copy buffer to image
    VkDevice vkDevice = device->GetHandle();
    u32 queueFamily = device->GetGraphicsQueue()->GetFamilyIndex();

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = queueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    VkCommandPool pool;
    vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &pool);

    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(vkDevice, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(
        cmd,
        stagingBuffer->GetHandle(),
        m_Image->GetHandle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(device->GetGraphicsQueue()->GetHandle(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device->GetGraphicsQueue()->GetHandle());

    vkFreeCommandBuffers(vkDevice, pool, 1, &cmd);
    vkDestroyCommandPool(vkDevice, pool, nullptr);

    // Generate mipmaps
    if (m_MipLevels > 1) {
        GenerateMipmaps(device, m_Image->GetHandle(), m_Format, width, height, m_MipLevels);
    } else {
        // Transition to shader read only layout
        TransitionImageLayout(
            device,
            m_Image->GetHandle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            m_MipLevels);
    }

    return true;
}

void Texture2D::GenerateMipmaps(VKDevice* device, VkImage image, VkFormat format, u32 width, u32 height, u32 mipLevels) {
    VkDevice vkDevice = device->GetHandle();
    u32 queueFamily = device->GetGraphicsQueue()->GetFamilyIndex();

    // Check if format supports linear blitting
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(device->GetPhysicalDeviceHandle(), format, &formatProps);

    if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        HC_CORE_WARN("Texture format does not support linear blitting, skipping mipmap generation");
        TransitionImageLayout(device, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
        return;
    }

    // Create command pool and buffer
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = queueFamily;

    VkCommandPool pool;
    vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &pool);

    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(vkDevice, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    i32 mipWidth = width;
    i32 mipHeight = height;

    for (u32 i = 1; i < mipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(cmd,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // Transition last mip level
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(device->GetGraphicsQueue()->GetHandle(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device->GetGraphicsQueue()->GetHandle());

    vkFreeCommandBuffers(vkDevice, pool, 1, &cmd);
    vkDestroyCommandPool(vkDevice, pool, nullptr);
}

void Texture2D::TransitionImageLayout(
    VKDevice* device,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    u32 mipLevels)
{
    VkDevice vkDevice = device->GetHandle();
    u32 queueFamily = device->GetGraphicsQueue()->GetFamilyIndex();

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = queueFamily;

    VkCommandPool pool;
    vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &pool);

    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(vkDevice, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        HC_CORE_ERROR("Unsupported layout transition");
        return;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(device->GetGraphicsQueue()->GetHandle(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device->GetGraphicsQueue()->GetHandle());

    vkFreeCommandBuffers(vkDevice, pool, 1, &cmd);
    vkDestroyCommandPool(vkDevice, pool, nullptr);
}

VkImage Texture2D::GetImage() const {
    return m_Image ? m_Image->GetHandle() : VK_NULL_HANDLE;
}

VkImageView Texture2D::GetImageView() const {
    return m_ImageView ? m_ImageView->GetHandle() : VK_NULL_HANDLE;
}

VkSampler Texture2D::GetSampler() const {
    return m_Sampler ? m_Sampler->GetHandle() : VK_NULL_HANDLE;
}

} // namespace happycat
