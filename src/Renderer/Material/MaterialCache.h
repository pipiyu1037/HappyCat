#pragma once

#include "Core/Utils/Types.h"
#include "Renderer/Material/Material.h"
#include <unordered_map>
#include <mutex>

namespace happycat {

class VKDevice;

// Singleton material cache for efficient material reuse
class MaterialCache {
public:
    static MaterialCache* Get();

    void Initialize(VKDevice* device);
    void Shutdown();

    // Create or get material by name
    Material* Create(const String& name);
    Material* CreateDefault(const String& name);

    // Get material if exists
    Material* Get(const String& name) const;

    // Remove material from cache
    void Release(const String& name);

    // Check if material exists
    bool Exists(const String& name) const;

    // Get number of cached materials
    size_t GetCacheSize() const { return m_MaterialCache.size(); }

    // Clear all cached materials
    void Clear();

private:
    MaterialCache() = default;
    ~MaterialCache() = default;

    MaterialCache(const MaterialCache&) = delete;
    MaterialCache& operator=(const MaterialCache&) = delete;

    VKDevice* m_Device = nullptr;
    HashMap<String, Scope<Material>> m_MaterialCache;
    mutable std::mutex m_Mutex;
};

} // namespace happycat
