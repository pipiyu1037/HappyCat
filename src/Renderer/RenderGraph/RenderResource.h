#pragma once

#include "Core/Utils/Types.h"
#include "RHI/RHITypes.h"
#include <string>

namespace happycat {

// Resource types
enum class ResourceType {
    Texture,
    Buffer
};

// Texture description for graph
struct GraphTextureDesc {
    std::string name;
    u32 width = 1;
    u32 height = 1;
    Format format = Format::RGBA8_SRGB;
    TextureUsage usage = TextureUsage::Sampled;
};

// Buffer description for graph
struct GraphBufferDesc {
    std::string name;
    u64 size = 1;
    BufferUsage usage = BufferUsage::Storage;
};

// Resource handle
struct TextureResource {
    TextureHandle handle;
    std::string name;
    GraphTextureDesc desc;
    bool transient = true;

    bool IsValid() const { return handle.IsValid(); }
};

struct BufferResource {
    BufferHandle handle;
    std::string name;
    GraphBufferDesc desc;
    bool transient = true;

    bool IsValid() const { return handle.IsValid(); }
};

// Resource access mode
enum class ResourceAccess {
    Read,
    Write,
    ReadWrite
};

} // namespace happycat
