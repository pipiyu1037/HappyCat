#pragma once

#include "Core/Utils/Types.h"
#include "Core/Math/MathTypes.h"
#include <vulkan/vulkan.h>

namespace happycat {

// Material texture slot indices - matches shader bindings
enum class MaterialTextureSlot : u32 {
    Albedo = 0,
    Normal = 1,
    MetallicRoughnessAO = 2,
    Emissive = 3,
    Count = 4
};

// Flags for which textures are used (matches shader)
enum class MaterialTextureFlags : u32 {
    None = 0,
    UseAlbedoTexture = 1 << 0,
    UseNormalTexture = 1 << 1,
    UseMetallicRoughnessAOTexture = 1 << 2,
    UseEmissiveTexture = 1 << 3,
    AlphaTest = 1 << 4
};

inline MaterialTextureFlags operator|(MaterialTextureFlags a, MaterialTextureFlags b) {
    return static_cast<MaterialTextureFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline MaterialTextureFlags operator&(MaterialTextureFlags a, MaterialTextureFlags b) {
    return static_cast<MaterialTextureFlags>(static_cast<u32>(a) & static_cast<u32>(b));
}

inline bool HasFlag(MaterialTextureFlags flags, MaterialTextureFlags flag) {
    return (flags & flag) == flag;
}

// PBR material parameters - matches shader uniform block
// Must be aligned to 16 bytes for GPU
struct alignas(16) PBRMaterialParams {
    glm::vec4 albedoColor = glm::vec4(1.0f);        // Base color when no albedo texture
    glm::vec3 emissiveColor = glm::vec3(0.0f);      // Emissive color
    float metallic = 0.5f;                          // Metallic factor [0, 1]
    float roughness = 0.5f;                         // Roughness factor [0, 1]
    float ao = 1.0f;                                // Ambient occlusion factor [0, 1]
    float alphaCutoff = 0.5f;                       // Alpha cutoff for alpha testing
    u32 flags = 0;                                  // MaterialTextureFlags bitmask
    float _padding[2] = {};                         // Padding to 16-byte alignment
};

// Texture creation descriptor
struct Texture2DDesc {
    String filePath;                                // Path to image file
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;     // Image format
    VkFilter magFilter = VK_FILTER_LINEAR;
    VkFilter minFilter = VK_FILTER_LINEAR;
    VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    bool generateMipmaps = true;
    const char* debugName = nullptr;
};

// Default material parameter presets
namespace MaterialPresets {
    constexpr PBRMaterialParams Default() {
        PBRMaterialParams params{};
        params.albedoColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
        params.metallic = 0.0f;
        params.roughness = 0.5f;
        params.ao = 1.0f;
        return params;
    }

    constexpr PBRMaterialParams Metal() {
        PBRMaterialParams params{};
        params.albedoColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        params.metallic = 1.0f;
        params.roughness = 0.2f;
        params.ao = 1.0f;
        return params;
    }

    constexpr PBRMaterialParams RoughMetal() {
        PBRMaterialParams params{};
        params.albedoColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
        params.metallic = 0.8f;
        params.roughness = 0.8f;
        params.ao = 1.0f;
        return params;
    }

    constexpr PBRMaterialParams Plastic() {
        PBRMaterialParams params{};
        params.albedoColor = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
        params.metallic = 0.0f;
        params.roughness = 0.3f;
        params.ao = 1.0f;
        return params;
    }

    constexpr PBRMaterialParams Emissive(glm::vec3 color) {
        PBRMaterialParams params{};
        params.albedoColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        params.emissiveColor = color;
        params.metallic = 0.0f;
        params.roughness = 1.0f;
        params.ao = 1.0f;
        return params;
    }
}

} // namespace happycat
