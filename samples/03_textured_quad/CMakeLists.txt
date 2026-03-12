# Textured Quad Demo

add_executable(TexturedQuadDemo
    main.cpp
    TexturedQuadApp.h
    TexturedQuadApp.cpp
)

target_link_libraries(TexturedQuadDemo PRIVATE
    HappyCatEngine
)

# Note: This copies the entire shaders directory including textured_quad.vert and textured_quad.frag
# The shader compiler will look for .spv files first,# If not found, it will compile from .vert/.frag source files

set_target_properties(TexturedQuadDemo PROPERTIES FOLDER "HappyCat/Samples")
