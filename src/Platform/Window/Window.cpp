#include "Window.h"
#include "HCWindow.h"

namespace happycat {

Window* Window::Create(const WindowCreateInfo& info) {
    return new HCWindow(info);
}

} // namespace happycat
