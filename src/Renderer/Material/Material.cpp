#include "Material.h"
#include "RHI/Vulkan/VKDevice.h"
#include "RHI/Vulkan/VKBuffer.h"
#include "RHI/Vulkan/VKDescriptorSetLayout.h"
#include "RHI/Vulkan/VKDescriptorPool.h"
#include "RHI/Vulkan/VKDescriptorSet.h"
#include "RHI/Vulkan/VKSampler.h"
#include "RHI/Vulkan/VKImageView.h"
#include "Core/Utils/Logger.h"

namespace happycat {

// Static default textures
static Scope<Texture2D> s_DefaultWhiteTexture;
static Scope<Texture2D> s_DefaultNormalTexture;
static Scope<Texture2D> s_DefaultMRATexture;
static Scope<Texture2D> s_DefaultEmissiveTexture;

Scope<Material> Material::Create(VKDevice* device, const String& name) {
    auto material = Scope<Material>(new Material());
    if (!material->Initialize(device, name)) {
        HC_CORE_ERROR("Failed to create material: {0}", name);
        return nullptr;
    }
    return material;
}

Scope<Material> Material::CreateDefault(VKDevice* device, const String& name) {
    auto material = Create(device, name);
    if (material) {
        material->SetParameters(MaterialPresets::Default());
    }
    return material;
}

Material::~Material() {
    // Resources cleaned up by smart pointers
}

bool Material::Initialize(VKDevice* device, const String& name) {
    m_Device = device;
    m_Name = name;
    m_Params = MaterialPresets::Default();

    // Create descriptor set layout for material
    // Binding 0: UniformBuffer (PBRMaterialParams)
    // Binding 1: CombinedImageSampler (Albedo)
    // Binding 2: CombinedImageSampler (Normal)
    // Binding 3: CombinedImageSampler (MetallicRoughnessAO)
    // Binding 4: CombinedImageSampler (Emissive)
    DescriptorSetLayoutDesc layoutDesc{};
    layoutDesc.bindings = {
        {0, DescriptorType::UniformBuffer, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {1, DescriptorType::CombinedImageSampler, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {2, DescriptorType::CombinedImageSampler, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {3, DescriptorType::CombinedImageSampler, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {4, DescriptorType::CombinedImageSampler, 1, VK_SHADER_STAGE_FRAGMENT_BIT}
    };

    m_DescriptorSetLayout = VKDescriptorSetLayout::Create(device, layoutDesc);
    if (!m_DescriptorSetLayout) {
        HC_CORE_ERROR("Failed to create descriptor set layout for material: {0}", name);
        return false;
    }

    HC_CORE_INFO("Created material: {0}", name);
    return true;
}

void Material::SetParameters(const PBRMaterialParams& params) {
    m_Params = params;
}

void Material::SetAlbedoTexture(Texture2D* texture) {
    m_Textures[static_cast<size_t>(MaterialTextureSlot::Albedo)] = texture;
    if (texture) {
        m_Params.flags |= static_cast<u32>(MaterialTextureFlags::UseAlbedoTexture);
    } else {
        m_Params.flags &= ~static_cast<u32>(MaterialTextureFlags::UseAlbedoTexture);
    }
    m_DescriptorSetsDirty = true;
}

void Material::SetNormalTexture(Texture2D* texture) {
    m_Textures[static_cast<size_t>(MaterialTextureSlot::Normal)] = texture;
    if (texture) {
        m_Params.flags |= static_cast<u32>(MaterialTextureFlags::UseNormalTexture);
    } else {
        m_Params.flags &= ~static_cast<u32>(MaterialTextureFlags::UseNormalTexture);
    }
    m_DescriptorSetsDirty = true;
}

void Material::SetMetallicRoughnessAOTexture(Texture2D* texture) {
    m_Textures[static_cast<size_t>(MaterialTextureSlot::MetallicRoughnessAO)] = texture;
    if (texture) {
        m_Params.flags |= static_cast<u32>(MaterialTextureFlags::UseMetallicRoughnessAOTexture);
    } else {
        m_Params.flags &= ~static_cast<u32>(MaterialTextureFlags::UseMetallicRoughnessAOTexture);
    }
    m_DescriptorSetsDirty = true;
}

void Material::SetEmissiveTexture(Texture2D* texture) {
    m_Textures[static_cast<size_t>(MaterialTextureSlot::Emissive)] = texture;
    if (texture) {
        m_Params.flags |= static_cast<u32>(MaterialTextureFlags::UseEmissiveTexture);
    } else {
        m_Params.flags &= ~static_cast<u32>(MaterialTextureFlags::UseEmissiveTexture);
    }
    m_DescriptorSetsDirty = true;
}

bool Material::CreateDescriptorPool(u32 framesInFlight) {
    // Create descriptor pool
    DescriptorPoolDesc poolDesc{};
    poolDesc.poolSizes = {
        {DescriptorType::UniformBuffer, framesInFlight},
        {DescriptorType::CombinedImageSampler, framesInFlight * 4}  // 4 texture bindings
    };
    poolDesc.maxSets = framesInFlight;

    m_DescriptorPool = VKDescriptorPool::Create(m_Device, poolDesc);
    if (!m_DescriptorPool) {
        HC_CORE_ERROR("Failed to create descriptor pool for material: {0}", m_Name);
        return false;
    }

    // Create uniform buffers
    m_UniformBuffers.resize(framesInFlight);
    for (u32 i = 0; i < framesInFlight; ++i) {
        m_UniformBuffers[i] = VKBuffer::Create(
            m_Device,
            sizeof(PBRMaterialParams),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (!m_UniformBuffers[i]) {
            HC_CORE_ERROR("Failed to create uniform buffer for material: {0}", m_Name);
            return false;
        }
    }

    // Allocate descriptor sets
    m_DescriptorSets.resize(framesInFlight);
    for (u32 i = 0; i < framesInFlight; ++i) {
        m_DescriptorSets[i] = m_DescriptorPool->Allocate(m_DescriptorSetLayout.get());
        if (!m_DescriptorSets[i]) {
            HC_CORE_ERROR("Failed to allocate descriptor set for material: {0}", m_Name);
            return false;
        }
    }

    // Create default textures if not already created
    if (!s_DefaultWhiteTexture) {
        CreateDefaultTextures(m_Device);
    }

    // Update descriptor sets
    UpdateDescriptorSets();

    return true;
}

void Material::CreateDefaultTextures() {
    // Create a 1x1 white pixel texture
    u8 whitePixel[4] = {255, 255, 255, 255};  // RGBA white

    Texture2DDesc desc{};
    desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    desc.generateMipmaps = false;

    s_DefaultWhiteTexture = Texture2D::CreateFromMemory(
        m_Device,
        whitePixel,
        1, 1, 4,
        desc);

    if (!s_DefaultWhiteTexture) {
        HC_CORE_ERROR("Failed to create default white texture");
        return;
    }

    // Create default normal map texture (neutral normal = (0.5, 0.5, 1.0))
    u8 normalPixel[4] = {128, 128, 255, 255};  // RGBA normal map color (pointing up in Z)

    Texture2DDesc normalDesc{};
    normalDesc.format = VK_FORMAT_R8G8B8A8_UNORM;
    normalDesc.generateMipmaps = false;

    s_DefaultNormalTexture = Texture2D::CreateFromMemory(
        m_Device,
        normalPixel,
        1, 1, 4,
        normalDesc);

    if (!s_DefaultNormalTexture) {
        HC_CORE_ERROR("Failed to create default normal texture");
        return;
    }

    // Create default MRA texture (metallic=0, roughness=0.5, ao=1)
    u8 mraPixel[4] = {0, 128, 255, 255};  // R=metallic(0), G=roughness(0.5), B=ao(1), A=1

    Texture2DDesc mraDesc{};
    mraDesc.format = VK_FORMAT_R8G8B8A8_UNORM;
    mraDesc.generateMipmaps = false;

    s_DefaultMRATexture = Texture2D::CreateFromMemory(
        m_Device,
        mraPixel,
        1, 1, 4,
        mraDesc);

    if (!s_DefaultMRATexture) {
        HC_CORE_ERROR("Failed to create default MRA texture");
        return;
    }

    // Create default emissive texture (black)
    u8 emissivePixel[4] = {0, 0, 0, 255};  // RGBA black

    Texture2DDesc emissiveDesc{};
    emissiveDesc.format = VK_FORMAT_R8G8B8A8_UNORM;
    emissiveDesc.generateMipmaps = false;

    s_DefaultEmissiveTexture = Texture2D::CreateFromMemory(
        m_Device,
        emissivePixel,
        1, 1, 4,
        emissiveDesc);

    if (!s_DefaultEmissiveTexture) {
        HC_CORE_ERROR("Failed to create default emissive texture");
        return;
    }

    HC_CORE_INFO("Created default textures for material system");
}

void Material::UpdateDescriptorSets() {
    if (!m_DescriptorSetsDirty || m_DescriptorSets.empty()) {
        return;
    }

    // Ensure default textures exist
    if (!s_DefaultWhiteTexture) {
        CreateDefaultTextures();
    }

    for (size_t i = 0; i < m_DescriptorSets.size(); ++i) {
        // Write uniform buffer
        m_DescriptorSets[i]->WriteUniformBuffer(m_Device, 0, m_UniformBuffers[i].get());

        // Write texture bindings - use default textures for unbound slots
        Texture2D* albedoTex = m_Textures[0] ? m_Textures[0] : s_DefaultWhiteTexture.get();
        Texture2D* normalTex = m_Textures[1] ? m_Textures[1] : s_DefaultNormalTexture.get();
        Texture2D* mraTex = m_Textures[2] ? m_Textures[2] : s_DefaultMRATexture.get();
        Texture2D* emissiveTex = m_Textures[3] ? m_Textures[3] : s_DefaultEmissiveTexture.get();

        m_DescriptorSets[i]->WriteCombinedImageSampler(
            m_Device, 1, albedoTex->GetImageViewWrapper(), albedoTex->GetSamplerWrapper());
        m_DescriptorSets[i]->WriteCombinedImageSampler(
            m_Device, 2, normalTex->GetImageViewWrapper(), normalTex->GetSamplerWrapper());
        m_DescriptorSets[i]->WriteCombinedImageSampler(
            m_Device, 3, mraTex->GetImageViewWrapper(), mraTex->GetSamplerWrapper());
        m_DescriptorSets[i]->WriteCombinedImageSampler(
            m_Device, 4, emissiveTex->GetImageViewWrapper(), emissiveTex->GetSamplerWrapper());
    }

    m_DescriptorSetsDirty = false;
    HC_CORE_TRACE("Updated descriptor sets for material: {0}", m_Name);
}

void Material::UpdateUniformBuffer(u32 frameIndex) {
    if (frameIndex >= m_UniformBuffers.size()) {
        return;
    }

    void* data = m_UniformBuffers[frameIndex]->Map();
    if (data) {
        memcpy(data, &m_Params, sizeof(PBRMaterialParams));
        m_UniformBuffers[frameIndex]->Unmap();
    }
}

VKDescriptorSet* Material::GetDescriptorSet(u32 frameIndex) const {
    if (frameIndex < m_DescriptorSets.size()) {
        return m_DescriptorSets[frameIndex].get();
    }
    return nullptr;
}

} // namespace happycat
