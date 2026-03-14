#include "MaterialCache.h"
#include "RHI/Vulkan/VKDevice.h"
#include "Core/Utils/Logger.h"

namespace happycat {

MaterialCache* MaterialCache::Get() {
    static MaterialCache instance;
    return &instance;
}

void MaterialCache::Initialize(VKDevice* device) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Device = device;
    HC_CORE_INFO("MaterialCache initialized");
}

void MaterialCache::Shutdown() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_MaterialCache.clear();
    m_Device = nullptr;
    HC_CORE_INFO("MaterialCache shutdown");
}

Material* MaterialCache::Create(const String& name) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    // Check if already exists
    auto it = m_MaterialCache.find(name);
    if (it != m_MaterialCache.end()) {
        HC_CORE_TRACE("Material already exists: {0}", name);
        return it->second.get();
    }

    // Create new material
    auto material = Material::Create(m_Device, name);
    if (!material) {
        HC_CORE_ERROR("Failed to create material: {0}", name);
        return nullptr;
    }

    Material* ptr = material.get();
    m_MaterialCache[name] = std::move(material);

    HC_CORE_TRACE("Created material: {0}", name);
    return ptr;
}

Material* MaterialCache::CreateDefault(const String& name) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    // Check if already exists
    auto it = m_MaterialCache.find(name);
    if (it != m_MaterialCache.end()) {
        HC_CORE_TRACE("Material already exists: {0}", name);
        return it->second.get();
    }

    // Create default material
    auto material = Material::CreateDefault(m_Device, name);
    if (!material) {
        HC_CORE_ERROR("Failed to create default material: {0}", name);
        return nullptr;
    }

    Material* ptr = material.get();
    m_MaterialCache[name] = std::move(material);

    HC_CORE_TRACE("Created default material: {0}", name);
    return ptr;
}

Material* MaterialCache::Get(const String& name) const {
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_MaterialCache.find(name);
    if (it != m_MaterialCache.end()) {
        return it->second.get();
    }
    return nullptr;
}

void MaterialCache::Release(const String& name) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_MaterialCache.find(name);
    if (it != m_MaterialCache.end()) {
        HC_CORE_TRACE("Releasing material: {0}", name);
        m_MaterialCache.erase(it);
    }
}

bool MaterialCache::Exists(const String& name) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_MaterialCache.find(name) != m_MaterialCache.end();
}

void MaterialCache::Clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_MaterialCache.clear();
    HC_CORE_INFO("MaterialCache cleared");
}

} // namespace happycat
