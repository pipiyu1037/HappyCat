#pragma once

#include "Core/Utils/Types.h"
#include "Core/Math/MathTypes.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>

namespace happycat {

// Format enumeration
enum class Format {
    Unknown = 0,

    // Color formats
    R8_UNORM,
    R8_SRGB,
    RG8_UNORM,
    RG8_SRGB,
    RGBA8_UNORM,
    RGBA8_SRGB,
    BGRA8_UNORM,
    BGRA8_SRGB,
    R16_UNORM,
    R16_SFLOAT,
    RG16_UNORM,
    RG16_SFLOAT,
    RGBA16_UNORM,
    RGBA16_SFLOAT,
    R32_SFLOAT,
    RG32_SFLOAT,
    RGBA32_SFLOAT,

    // Depth/Stencil formats
    D16_UNORM,
    D24_UNORM_S8_UINT,
    D32_SFLOAT,
    D32_SFLOAT_S8_UINT
};

// Convert to Vulkan format
inline VkFormat ToVkFormat(Format format) {
    switch (format) {
        case Format::R8_UNORM:             return VK_FORMAT_R8_UNORM;
        case Format::R8_SRGB:              return VK_FORMAT_R8_SRGB;
        case Format::RG8_UNORM:            return VK_FORMAT_R8G8_UNORM;
        case Format::RG8_SRGB:             return VK_FORMAT_R8G8_SRGB;
        case Format::RGBA8_UNORM:          return VK_FORMAT_R8G8B8A8_UNORM;
        case Format::RGBA8_SRGB:           return VK_FORMAT_R8G8B8A8_SRGB;
        case Format::BGRA8_UNORM:          return VK_FORMAT_B8G8R8A8_UNORM;
        case Format::BGRA8_SRGB:           return VK_FORMAT_B8G8R8A8_SRGB;
        case Format::R16_UNORM:            return VK_FORMAT_R16_UNORM;
        case Format::R16_SFLOAT:           return VK_FORMAT_R16_SFLOAT;
        case Format::RG16_UNORM:           return VK_FORMAT_R16G16_UNORM;
        case Format::RG16_SFLOAT:          return VK_FORMAT_R16G16_SFLOAT;
        case Format::RGBA16_UNORM:         return VK_FORMAT_R16G16B16A16_UNORM;
        case Format::RGBA16_SFLOAT:        return VK_FORMAT_R16G16B16A16_SFLOAT;
        case Format::R32_SFLOAT:           return VK_FORMAT_R32_SFLOAT;
        case Format::RG32_SFLOAT:          return VK_FORMAT_R32G32_SFLOAT;
        case Format::RGBA32_SFLOAT:        return VK_FORMAT_R32G32B32A32_SFLOAT;
        case Format::D16_UNORM:            return VK_FORMAT_D16_UNORM;
        case Format::D24_UNORM_S8_UINT:    return VK_FORMAT_D24_UNORM_S8_UINT;
        case Format::D32_SFLOAT:           return VK_FORMAT_D32_SFLOAT;
        case Format::D32_SFLOAT_S8_UINT:   return VK_FORMAT_D32_SFLOAT_S8_UINT;
        default:                           return VK_FORMAT_UNDEFINED;
    }
}

inline Format FromVkFormat(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8_UNORM:                  return Format::R8_UNORM;
        case VK_FORMAT_R8_SRGB:                   return Format::R8_SRGB;
        case VK_FORMAT_R8G8_UNORM:                return Format::RG8_UNORM;
        case VK_FORMAT_R8G8_SRGB:                 return Format::RG8_SRGB;
        case VK_FORMAT_R8G8B8A8_UNORM:            return Format::RGBA8_UNORM;
        case VK_FORMAT_R8G8B8A8_SRGB:             return Format::RGBA8_SRGB;
        case VK_FORMAT_B8G8R8A8_UNORM:            return Format::BGRA8_UNORM;
        case VK_FORMAT_B8G8R8A8_SRGB:             return Format::BGRA8_SRGB;
        case VK_FORMAT_R16_UNORM:                 return Format::R16_UNORM;
        case VK_FORMAT_R16_SFLOAT:                return Format::R16_SFLOAT;
        case VK_FORMAT_R16G16_UNORM:              return Format::RG16_UNORM;
        case VK_FORMAT_R16G16_SFLOAT:             return Format::RG16_SFLOAT;
        case VK_FORMAT_R16G16B16A16_UNORM:        return Format::RGBA16_UNORM;
        case VK_FORMAT_R16G16B16A16_SFLOAT:       return Format::RGBA16_SFLOAT;
        case VK_FORMAT_R32_SFLOAT:                return Format::R32_SFLOAT;
        case VK_FORMAT_R32G32_SFLOAT:             return Format::RG32_SFLOAT;
        case VK_FORMAT_R32G32B32A32_SFLOAT:       return Format::RGBA32_SFLOAT;
        case VK_FORMAT_D16_UNORM:                 return Format::D16_UNORM;
        case VK_FORMAT_D24_UNORM_S8_UINT:         return Format::D24_UNORM_S8_UINT;
        case VK_FORMAT_D32_SFLOAT:                return Format::D32_SFLOAT;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:        return Format::D32_SFLOAT_S8_UINT;
        default:                                  return Format::Unknown;
    }
}

// Buffer usage flags
enum class BufferUsage : u32 {
    None = 0,
    Vertex = 1 << 0,
    Index = 1 << 1,
    Uniform = 1 << 2,
    Storage = 1 << 3,
    TransferSrc = 1 << 4,
    TransferDst = 1 << 5,
    Indirect = 1 << 6
};

// Bitwise operators for BufferUsage
inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<u32>(a) | static_cast<u32>(b));
}
inline BufferUsage operator&(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<u32>(a) & static_cast<u32>(b));
}
inline BufferUsage& operator|=(BufferUsage& a, BufferUsage b) {
    a = a | b;
    return a;
}
inline bool HasFlag(BufferUsage flags, BufferUsage flag) {
    return (flags & flag) == flag;
}

// Texture usage flags
enum class TextureUsage : u32 {
    None = 0,
    Sampled = 1 << 0,
    ColorAttachment = 1 << 1,
    DepthStencilAttachment = 1 << 2,
    Storage = 1 << 3,
    TransferSrc = 1 << 4,
    TransferDst = 1 << 5
};

// Bitwise operators for TextureUsage
inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(static_cast<u32>(a) | static_cast<u32>(b));
}
inline TextureUsage operator&(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(static_cast<u32>(a) & static_cast<u32>(b));
}
inline TextureUsage& operator|=(TextureUsage& a, TextureUsage b) {
    a = a | b;
    return a;
}
inline bool HasFlag(TextureUsage flags, TextureUsage flag) {
    return (flags & flag) == flag;
}

// Queue types
enum class QueueType : u32 {
    Graphics = 0,
    Compute,
    Transfer,
    Count
};

// Buffer description
struct BufferDesc {
    u64 size = 0;
    BufferUsage usage = BufferUsage::None;
    bool deviceLocal = true;
    const char* debugName = nullptr;
};

// Texture description
struct TextureDesc {
    u32 width = 1;
    u32 height = 1;
    u32 depth = 1;
    u32 mipLevels = 1;
    u32 arrayLayers = 1;
    Format format = Format::RGBA8_UNORM;
    TextureUsage usage = TextureUsage::None;
    const char* debugName = nullptr;
};

// Shader stage
enum class ShaderStage : u32 {
    Vertex = 0,
    Fragment,
    Geometry,
    Compute,
    Count
};

// Vertex input description
struct VertexInputBinding {
    u32 binding = 0;
    u32 stride = 0;
    VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
};

struct VertexInputAttribute {
    u32 location = 0;
    u32 binding = 0;
    Format format = Format::Unknown;
    u32 offset = 0;
};

// Pipeline description
struct GraphicsPipelineDesc {
    // Shaders
    std::string vertexShader;
    std::string fragmentShader;

    // Vertex input
    Array<VertexInputBinding> vertexBindings;
    Array<VertexInputAttribute> vertexAttributes;

    // Input assembly
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    bool primitiveRestartEnable = false;

    // Rasterization
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    f32 lineWidth = 1.0f;

    // Blending
    bool blendEnable = false;
    VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;

    // Depth/Stencil
    bool depthTestEnable = false;
    bool depthWriteEnable = true;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;

    // Render pass
    VkRenderPass renderPass = VK_NULL_HANDLE;
    u32 subpass = 0;
};

// Resource handle
using ResourceHandle = u64;
constexpr ResourceHandle INVALID_RESOURCE_HANDLE = 0;

// Texture handle
struct TextureHandle {
    ResourceHandle handle = INVALID_RESOURCE_HANDLE;

    bool IsValid() const { return handle != INVALID_RESOURCE_HANDLE; }
    bool operator==(const TextureHandle& other) const { return handle == other.handle; }
    bool operator!=(const TextureHandle& other) const { return handle != other.handle; }
};

// Buffer handle
struct BufferHandle {
    ResourceHandle handle = INVALID_RESOURCE_HANDLE;

    bool IsValid() const { return handle != INVALID_RESOURCE_HANDLE; }
    bool operator==(const BufferHandle& other) const { return handle == other.handle; }
    bool operator!=(const BufferHandle& other) const { return handle != other.handle; }
};

} // namespace happycat
