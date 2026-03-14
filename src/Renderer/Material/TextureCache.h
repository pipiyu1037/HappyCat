#pragma once

#include "Core/Utils/Types.h"
#include "Renderer/Material/Texture2D.h"
#include <unordered_map>
#include <mutex>

namespace happycat {

class VKDevice;

// Singleton texture cache for efficient texture reuse
class TextureCache {
public:
    static TextureCache* Get();

    void Initialize(VKDevice* device);
    void Shutdown();

    // Load texture from file (cached)
    Texture2D* Load(const String& filePath, const Texture2DDesc& desc = Texture2DDesc{});

    // Get texture if already loaded
    Texture2D* Get(const String& filePath) const;

    // Release texture reference (removes from cache)
    void Release(const String& filePath);

    // Check if texture is loaded
    bool IsLoaded(const String& filePath) const;

    // Get number of cached textures
    size_t GetCacheSize() const { return m_TextureCache.size(); }

    // Clear all cached textures
    void Clear();

private:
    TextureCache() = default;
    ~TextureCache() = default;

    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    VKDevice* m_Device = nullptr;
    HashMap<String, Scope<Texture2D>> m_TextureCache;
    mutable std::mutex m_Mutex;
};

} // namespace happycat
