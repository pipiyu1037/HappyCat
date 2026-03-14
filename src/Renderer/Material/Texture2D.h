#pragma once

#include "Core/Utils/Types.h"
#include "Renderer/Material/MaterialTypes.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace happycat {

class VKDevice;
class VKImage;
class VKImageView;
class VKSampler;
class VKBuffer;
class VKCommandBuffer;

// 2D Texture resource wrapping VKImage, VKImageView, and VKSampler
class Texture2D {
public:
    static Scope<Texture2D> Create(VKDevice* device, const Texture2DDesc& desc);
    static Scope<Texture2D> CreateFromMemory(
        VKDevice* device,
        const u8* data,
        u32 width,
        u32 height,
        u32 channels,
        const Texture2DDesc& desc);

    // Create a 1x1 white texture for default bindings
    static Scope<Texture2D> CreateDefaultWhite(VKDevice* device);

    // Create a 1x1 flat normal texture (0.5, 0.5, 1.0)
    static Scope<Texture2D> CreateDefaultNormal(VKDevice* device);

    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&&) noexcept = default;
    Texture2D& operator=(Texture2D&&) noexcept = default;

    // Getters
    VkImage GetImage() const;
    VkImageView GetImageView() const;
    VkSampler GetSampler() const;

    // Get wrapper pointers (for descriptor set binding)
    VKImageView* GetImageViewWrapper() const { return m_ImageView.get(); }
    VKSampler* GetSamplerWrapper() const { return m_Sampler.get(); }
    u32 GetWidth() const { return m_Width; }
    u32 GetHeight() const { return m_Height; }
    u32 GetMipLevels() const { return m_MipLevels; }
    VkFormat GetFormat() const { return m_Format; }
    const String& GetFilePath() const { return m_FilePath; }

private:
    Texture2D() = default;

    bool Initialize(VKDevice* device, const Texture2DDesc& desc);
    bool UploadData(VKDevice* device, const u8* pixelData, u32 width, u32 height, u32 channels);
    void GenerateMipmaps(VKDevice* device, VkImage image, VkFormat format, u32 width, u32 height, u32 mipLevels);
    void TransitionImageLayout(
        VKDevice* device,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        u32 mipLevels);

    VKDevice* m_Device = nullptr;
    Scope<VKImage> m_Image;
    Scope<VKImageView> m_ImageView;
    Scope<VKSampler> m_Sampler;

    u32 m_Width = 0;
    u32 m_Height = 0;
    u32 m_MipLevels = 1;
    VkFormat m_Format = VK_FORMAT_R8G8B8A8_SRGB;
    String m_FilePath;
};

} // namespace happycat
