#pragma once

#include "Types.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <sstream>

namespace happycat {

enum class ResourceType {
    Buffer,
    Image,
    ImageView,
    Pipeline,
    PipelineLayout,
    RenderPass,
    Framebuffer,
    CommandPool,
    CommandBuffer,
    Semaphore,
    Fence,
    DescriptorPool,
    DescriptorSetLayout,
    ShaderModule,
    Swapchain,
    Surface,
    Sampler,
    Other
};

const char* ResourceTypeToString(ResourceType type);

struct ResourceInfo {
    ResourceType type;
    std::string name;
    std::string file;
    int line;
    size_t size;  // Optional size for buffers/images
};

class ResourceTracker {
public:
    static ResourceTracker& Get();

    void TrackResource(void* handle, ResourceType type, const std::string& name,
                       const char* file, int line, size_t size = 0);
    void UntrackResource(void* handle);

    void DumpLeaks() const;
    bool HasLeaks() const;
    size_t GetResourceCount() const;

    // Enable/disable tracking (useful for release builds)
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

private:
    ResourceTracker() = default;
    ~ResourceTracker();

    ResourceTracker(const ResourceTracker&) = delete;
    ResourceTracker& operator=(const ResourceTracker&) = delete;

    std::unordered_map<void*, ResourceInfo> m_Resources;
    mutable std::mutex m_Mutex;
    bool m_Enabled = true;
};

// Helper macros for tracking
#define HC_TRACK_RESOURCE(handle, type, name, size) \
    ::happycat::ResourceTracker::Get().TrackResource(handle, type, name, __FILE__, __LINE__, size)

#define HC_UNTRACK_RESOURCE(handle) \
    ::happycat::ResourceTracker::Get().UntrackResource(handle)

// Lightweight macros that can be disabled in release
#ifdef HC_ENABLE_RESOURCE_TRACKING
    #define HC_TRACK_RESOURCE_DBG(handle, type, name, size) HC_TRACK_RESOURCE(handle, type, name, size)
    #define HC_UNTRACK_RESOURCE_DBG(handle) HC_UNTRACK_RESOURCE(handle)
#else
    #define HC_TRACK_RESOURCE_DBG(handle, type, name, size) ((void)0)
    #define HC_UNTRACK_RESOURCE_DBG(handle) ((void)0)
#endif

} // namespace happycat
