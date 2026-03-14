#pragma once

#include "Core/Utils/Types.h"
#include "Renderer/Material/MaterialTypes.h"
#include "Renderer/Material/Texture2D.h"
#include "RHI/Vulkan/VKDescriptorPool.h"
#include "RHI/Vulkan/VKDevice.h"
#include "RHI/Vulkan/VKBuffer.h"
#include "RHI/Vulkan/VKDescriptorSetLayout.h"
#include "RHI/Vulkan/VKDescriptorSet.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <array>
#include <vector>

namespace happycat {

// Forward declarations
class VKSampler;

// PBR Material class managing textures and material parameters
class Material {
public:
    static Scope<Material> Create(VKDevice* device, const String& name = "Unnamed");
    static Scope<Material> CreateDefault(VKDevice* device, const String& name = "Default");

    ~Material();

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(Material&&) noexcept = default;
    Material& operator=(Material&&) noexcept = default;

    // Material parameters
    void SetParameters(const PBRMaterialParams& params);
    const PBRMaterialParams& GetParameters() const { return m_Params; }

    // Texture setters
    void SetAlbedoTexture(Texture2D* texture);
    void SetNormalTexture(Texture2D* texture);
    void SetMetallicRoughnessAOTexture(Texture2D* texture);
    void SetEmissiveTexture(Texture2D* texture);

    // Get textures
    Texture2D* GetAlbedoTexture() const { return m_Textures[0]; }
    Texture2D* GetNormalTexture() const { return m_Textures[1]; }
    Texture2D* GetMetallicRoughnessAOTexture() const { return m_Textures[2]; }
    Texture2D* GetEmissiveTexture() const { return m_Textures[3]; }

    // Descriptor set management
    VKDescriptorSetLayout* GetDescriptorSetLayout() const { return m_DescriptorSetLayout.get(); }
    VKDescriptorSet* GetDescriptorSet(u32 frameIndex) const;

    // Create descriptor pool and allocate sets for frames in flight
    bool CreateDescriptorPool(u32 framesInFlight);

    // Update descriptor sets (call after setting textures)
    void UpdateDescriptorSets();

    // Update uniform buffer for specific frame
    void UpdateUniformBuffer(u32 frameIndex);

    // Get material name
    const String& GetName() const { return m_Name; }

    // Get/Set blend mode
    bool IsTransparent() const { return m_IsTransparent; }
    void SetTransparent(bool transparent) { m_IsTransparent = transparent; }

    // Cleanup static default textures (call before shutting down Vulkan)
    static void CleanupDefaultTextures();

private:
    Material() = default;
    bool Initialize(VKDevice* device, const String& name);
    static void CreateDefaultTextures(VKDevice* device);

    VKDevice* m_Device = nullptr;
    String m_Name;

    // Material parameters
    PBRMaterialParams m_Params{};

    // Textures (indexed by MaterialTextureSlot)
    std::array<Texture2D*, 4> m_Textures = {};

    // Uniform buffers (one per frame in flight)
    std::vector<Scope<VKBuffer>> m_UniformBuffers;

    // Descriptor resources
    Scope<VKDescriptorSetLayout> m_DescriptorSetLayout;
    Scope<VKDescriptorPool> m_DescriptorPool;
    std::vector<Scope<VKDescriptorSet>> m_DescriptorSets;

    // Flags
    bool m_IsTransparent = false;
    bool m_DescriptorSetsDirty = true;
};

} // namespace happycat
