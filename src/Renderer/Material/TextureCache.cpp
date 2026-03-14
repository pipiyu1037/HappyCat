#include "TextureCache.h"
#include "RHI/Vulkan/VKDevice.h"
#include "Core/Utils/Logger.h"

namespace happycat {

TextureCache* TextureCache::Get() {
    static TextureCache instance;
    return &instance;
}

void TextureCache::Initialize(VKDevice* device) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Device = device;
    HC_CORE_INFO("TextureCache initialized");
}

void TextureCache::Shutdown() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_TextureCache.clear();
    m_Device = nullptr;
    HC_CORE_INFO("TextureCache shutdown");
}

Texture2D* TextureCache::Load(const String& filePath, const Texture2DDesc& desc) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    // Check if already loaded
    auto it = m_TextureCache.find(filePath);
    if (it != m_TextureCache.end()) {
        HC_CORE_TRACE("Texture cache hit: {0}", filePath);
        return it->second.get();
    }

    // Create new texture
    HC_CORE_TRACE("Texture cache miss, loading: {0}", filePath);

    Texture2DDesc loadDesc = desc;
    loadDesc.filePath = filePath;

    auto texture = Texture2D::Create(m_Device, loadDesc);
    if (!texture) {
        HC_CORE_ERROR("Failed to load texture: {0}", filePath);
        return nullptr;
    }

    Texture2D* ptr = texture.get();
    m_TextureCache[filePath] = std::move(texture);

    return ptr;
}

Texture2D* TextureCache::Get(const String& filePath) const {
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_TextureCache.find(filePath);
    if (it != m_TextureCache.end()) {
        return it->second.get();
    }
    return nullptr;
}

void TextureCache::Release(const String& filePath) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_TextureCache.find(filePath);
    if (it != m_TextureCache.end()) {
        HC_CORE_TRACE("Releasing texture: {0}", filePath);
        m_TextureCache.erase(it);
    }
}

bool TextureCache::IsLoaded(const String& filePath) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_TextureCache.find(filePath) != m_TextureCache.end();
}

void TextureCache::Clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_TextureCache.clear();
    HC_CORE_INFO("TextureCache cleared");
}

} // namespace happycat
