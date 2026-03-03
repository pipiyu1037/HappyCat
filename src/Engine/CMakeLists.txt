# HappyCat Engine Module

add_library(HappyCatEngine STATIC
    Application.h
    Application.cpp
    Engine.h
    Engine.cpp
    FrameContext.h
    FrameContext.cpp
)

target_include_directories(HappyCatEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HappyCatEngine PUBLIC
    HappyCatCore
    HappyCatRHI
    HappyCatRenderer
    HappyCatPlatform
)

set_target_properties(HappyCatEngine PROPERTIES FOLDER "HappyCat/Modules")
