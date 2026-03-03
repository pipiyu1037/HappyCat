# HappyCat Platform Module

add_library(HappyCatPlatform STATIC
    # Window
    Window/Window.h
    Window/Window.cpp
    Window/GLFWWindow.h
    Window/GLFWWindow.cpp

    # Input
    Input/InputManager.h
    Input/InputManager.cpp
)

target_include_directories(HappyCatPlatform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HappyCatPlatform PUBLIC
    HappyCatCore
    glfw
    Vulkan::Vulkan
)

set_target_properties(HappyCatPlatform PROPERTIES FOLDER "HappyCat/Modules")
