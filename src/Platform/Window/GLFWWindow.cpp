#include "GLFWWindow.h"
#include "Core/Utils/Logger.h"

namespace happycat {

u32 GLFWWindow::s_WindowCount = 0;

void GLFWWindow::InitGLFW() {
    if (s_WindowCount == 0) {
        if (!glfwInit()) {
            HC_CORE_CRITICAL("Failed to initialize GLFW");
        }
        glfwSetErrorCallback([](int error, const char* description) {
            HC_CORE_ERROR("GLFW Error: {0}", description);
        });
    }
    s_WindowCount++;
}

void GLFWWindow::ShutdownGLFW() {
    s_WindowCount--;
    if (s_WindowCount == 0) {
        glfwTerminate();
    }
}

GLFWWindow::GLFWWindow(const WindowCreateInfo& info) {
    InitGLFW();

    // GLFW hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, info.resizable ? GLFW_TRUE : GLFW_FALSE);

    // Create window
    m_Window = glfwCreateWindow(
        static_cast<i32>(info.width),
        static_cast<i32>(info.height),
        info.title.c_str(),
        nullptr, nullptr
    );

    if (!m_Window) {
        HC_CORE_CRITICAL("Failed to create GLFW window");
    }

    m_Width = info.width;
    m_Height = info.height;

    // Set callbacks
    glfwSetWindowUserPointer(m_Window, this);

    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, i32 width, i32 height) {
        auto* w = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (w) {
            w->m_Width = static_cast<u32>(width);
            w->m_Height = static_cast<u32>(height);
            if (w->m_ResizeCallback) {
                w->m_ResizeCallback(static_cast<u32>(width), static_cast<u32>(height));
            }
        }
    });

    glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
        auto* w = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (w && w->m_CloseCallback) {
            w->m_CloseCallback();
        }
    });

    glfwSetKeyCallback(m_Window, [](GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods) {
        auto* w = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (w && w->m_KeyCallback) {
            w->m_KeyCallback(key, scancode, action, mods);
        }
    });

    glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, f64 xpos, f64 ypos) {
        auto* w = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (w && w->m_MouseCallback) {
            w->m_MouseCallback(xpos, ypos);
        }
    });

    glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, i32 button, i32 action, i32 mods) {
        auto* w = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (w && w->m_MouseButtonCallback) {
            w->m_MouseButtonCallback(button, action, mods);
        }
    });

    glfwSetScrollCallback(m_Window, [](GLFWwindow* window, f64 xoffset, f64 yoffset) {
        auto* w = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (w && w->m_ScrollCallback) {
            w->m_ScrollCallback(xoffset, yoffset);
        }
    });

    HC_CORE_INFO("GLFW window created: {0}x{1}", m_Width, m_Height);
}

GLFWWindow::~GLFWWindow() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    ShutdownGLFW();
    HC_CORE_INFO("GLFW window destroyed");
}

void GLFWWindow::PollEvents() {
    glfwPollEvents();
}

bool GLFWWindow::ShouldClose() const {
    return glfwWindowShouldClose(m_Window) == GLFW_TRUE;
}

void GLFWWindow::GetFramebufferSize(u32& width, u32& height) const {
    i32 w, h;
    glfwGetFramebufferSize(m_Window, &w, &h);
    width = static_cast<u32>(w);
    height = static_cast<u32>(h);
}

VkSurfaceKHR GLFWWindow::CreateSurface(VkInstance instance) const {
    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, m_Window, nullptr, &surface));
    return surface;
}

void GLFWWindow::SetResizeCallback(ResizeCallback callback) {
    m_ResizeCallback = callback;
}

void GLFWWindow::SetCloseCallback(CloseCallback callback) {
    m_CloseCallback = callback;
}

void GLFWWindow::SetKeyCallback(KeyCallback callback) {
    m_KeyCallback = callback;
}

void GLFWWindow::SetMouseCallback(MouseCallback callback) {
    m_MouseCallback = callback;
}

void GLFWWindow::SetMouseButtonCallback(MouseButtonCallback callback) {
    m_MouseButtonCallback = callback;
}

void GLFWWindow::SetScrollCallback(ScrollCallback callback) {
    m_ScrollCallback = callback;
}

} // namespace happycat
