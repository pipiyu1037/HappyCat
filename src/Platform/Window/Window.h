#pragma once

#include "Core/Utils/Types.h"
#include <string>
#include <functional>
#include "RHI/RHIDefs.h"

namespace happycat {

struct WindowCreateInfo {
    std::string title = "HappyCat";
    u32 width = 1280;
    u32 height = 720;
    bool fullscreen = false;
    bool resizable = true;
    bool vsync = true;
};

// Callbacks
using ResizeCallback = std::function<void(u32, u32)>;
using CloseCallback = std::function<void()>;
using KeyCallback = std::function<void(i32, i32, i32, i32)>;
using MouseCallback = std::function<void(f64, f64)>;
using MouseButtonCallback = std::function<void(i32, i32, i32)>;
using ScrollCallback = std::function<void(f64, f64)>;

class Window {
public:
    virtual ~Window() = default;

    // Events
    virtual void PollEvents() = 0;
    virtual bool ShouldClose() const = 0;

    // Size
    virtual u32 GetWidth() const = 0;
    virtual u32 GetHeight() const = 0;
    virtual void GetFramebufferSize(u32& width, u32& height) const = 0;

    // Vulkan Surface
    virtual VkSurfaceKHR CreateSurface(VkInstance instance) const = 0;

    // Callbacks
    virtual void SetResizeCallback(ResizeCallback callback) = 0;
    virtual void SetCloseCallback(CloseCallback callback) = 0;
    virtual void SetKeyCallback(KeyCallback callback) = 0;
    virtual void SetMouseCallback(MouseCallback callback) = 0;
    virtual void SetMouseButtonCallback(MouseButtonCallback callback) = 0;
    virtual void SetScrollCallback(ScrollCallback callback) = 0;

    // Native handle
    virtual void* GetNativeHandle() const = 0;

    // Factory
    static Window* Create(const WindowCreateInfo& info);
};

} // namespace happycat
