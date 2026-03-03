#pragma once

#include "Window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace happycat {

class GLFWWindow : public Window {
public:
    explicit GLFWWindow(const WindowCreateInfo& info);
    ~GLFWWindow() override;

    // Events
    void PollEvents() override;
    bool ShouldClose() const override;

    // Size
    u32 GetWidth() const override { return m_Width; }
    u32 GetHeight() const override { return m_Height; }
    void GetFramebufferSize(u32& width, u32& height) const override;

    // Vulkan Surface
    VkSurfaceKHR CreateSurface(VkInstance instance) const override;

    // Callbacks
    void SetResizeCallback(ResizeCallback callback) override;
    void SetCloseCallback(CloseCallback callback) override;
    void SetKeyCallback(KeyCallback callback) override;
    void SetMouseCallback(MouseCallback callback) override;
    void SetMouseButtonCallback(MouseButtonCallback callback) override;
    void SetScrollCallback(ScrollCallback callback) override;

    // Native handle
    void* GetNativeHandle() const override { return m_Window; }

private:
    static void InitGLFW();
    static void ShutdownGLFW();

    GLFWwindow* m_Window = nullptr;
    u32 m_Width;
    u32 m_Height;

    ResizeCallback m_ResizeCallback;
    CloseCallback m_CloseCallback;
    KeyCallback m_KeyCallback;
    MouseCallback m_MouseCallback;
    MouseButtonCallback m_MouseButtonCallback;
    ScrollCallback m_ScrollCallback;

    // Static GLFW state
    static u32 s_WindowCount;
};

} // namespace happycat
