#pragma once

#include "Engine/Application.h"
#include "TrianglePass.h"
#include <memory>

namespace happycat {

class RenderGraphApp : public Application {
public:
    RenderGraphApp(const ApplicationConfig& config);
    ~RenderGraphApp();

protected:
    bool OnInit() override;
    void OnShutdown() override;
    void OnUpdate(f32 deltaTime) override;
    void OnRender() override;
    void OnResize(u32 width, u32 height) override;

private:
    std::unique_ptr<TrianglePass> m_TrianglePass;
};

} // namespace happycat
