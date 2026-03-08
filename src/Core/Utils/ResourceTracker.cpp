#include "ResourceTracker.h"
#include "Logger.h"
#include <iostream>

namespace happycat {

const char* ResourceTypeToString(ResourceType type) {
    switch (type) {
        case ResourceType::Buffer: return "Buffer";
        case ResourceType::Image: return "Image";
        case ResourceType::ImageView: return "ImageView";
        case ResourceType::Pipeline: return "Pipeline";
        case ResourceType::PipelineLayout: return "PipelineLayout";
        case ResourceType::RenderPass: return "RenderPass";
        case ResourceType::Framebuffer: return "Framebuffer";
        case ResourceType::CommandPool: return "CommandPool";
        case ResourceType::CommandBuffer: return "CommandBuffer";
        case ResourceType::Semaphore: return "Semaphore";
        case ResourceType::Fence: return "Fence";
        case ResourceType::DescriptorPool: return "DescriptorPool";
        case ResourceType::DescriptorSetLayout: return "DescriptorSetLayout";
        case ResourceType::ShaderModule: return "ShaderModule";
        case ResourceType::Swapchain: return "Swapchain";
        case ResourceType::Surface: return "Surface";
        case ResourceType::Sampler: return "Sampler";
        case ResourceType::Other: return "Other";
        default: return "Unknown";
    }
}

ResourceTracker& ResourceTracker::Get() {
    static ResourceTracker instance;
    return instance;
}

ResourceTracker::~ResourceTracker() {
    if (m_Enabled) {
        DumpLeaks();
    }
}

void ResourceTracker::TrackResource(void* handle, ResourceType type, const std::string& name,
                                     const char* file, int line, size_t size) {
    if (!m_Enabled || !handle) return;

    std::lock_guard<std::mutex> lock(m_Mutex);

    ResourceInfo info{};
    info.type = type;
    info.name = name;
    info.file = file;
    info.line = line;
    info.size = size;

    m_Resources[handle] = info;
}

void ResourceTracker::UntrackResource(void* handle) {
    if (!m_Enabled || !handle) return;

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Resources.erase(handle);
}

void ResourceTracker::DumpLeaks() const {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_Resources.empty()) {
        HC_CORE_INFO("ResourceTracker: No resource leaks detected");
        return;
    }

    HC_CORE_ERROR("ResourceTracker: Detected {0} resource leak(s):", m_Resources.size());

    for (const auto& [handle, info] : m_Resources) {
        std::stringstream ss;
        ss << "  [" << ResourceTypeToString(info.type) << "] "
           << info.name << " (" << info.file << ":" << info.line << ")";
        if (info.size > 0) {
            ss << " size=" << info.size;
        }
        HC_CORE_ERROR("{0}", ss.str());
    }
}

bool ResourceTracker::HasLeaks() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return !m_Resources.empty();
}

size_t ResourceTracker::GetResourceCount() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Resources.size();
}

} // namespace happycat
