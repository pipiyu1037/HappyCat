#include "Window.h"
#include "GLFWWindow.h"

namespace happycat {

Window* Window::Create(const WindowCreateInfo& info) {
    return new GLFWWindow(info);
}

} // namespace happycat
