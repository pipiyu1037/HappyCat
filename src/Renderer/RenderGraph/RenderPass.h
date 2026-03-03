#pragma once

#include "Core/Utils/Types.h"
#include "RenderResource.h"
#include <string>
#include <vector>

namespace happycat {

class ResourceBuilder;
class DependencyBuilder;
class VKCommandBuffer;

// Render pass context (passed during execution)
struct RenderPassContext {
    u32 width;
    u32 height;
    u32 frameIndex;
};

// Base render pass class
class RenderPass {
public:
    virtual ~RenderPass() = default;

    // Pass information
    virtual const char* GetName() const = 0;
    virtual u32 GetPriority() const { return 1; }

    // Resource declaration
    virtual void DeclareResources(ResourceBuilder& builder) {}

    // Dependency declaration
    virtual void DeclareDependencies(DependencyBuilder& builder) {}

    // Execution
    virtual bool ShouldExecute() const { return true; }
    virtual void Execute(RenderPassContext& ctx, VKCommandBuffer& cmd) = 0;

protected:
    std::vector<TextureHandle> m_ColorAttachments;
    TextureHandle m_DepthAttachment;
    bool m_HasDepth = false;
};

} // namespace happycat
