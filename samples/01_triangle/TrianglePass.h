#pragma once

#include "Renderer/RenderGraph/RenderPass.h"

namespace happycat {

// Simple triangle pass (placeholder)
class TrianglePass : public RenderPass {
public:
    TrianglePass() = default;

    const char* GetName() const override { return "TrianglePass"; }

    void Execute(RenderPassContext& ctx, VKCommandBuffer& cmd) override;
};

} // namespace happycat
