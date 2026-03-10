# Render Graph Demo

add_executable(RenderGraphDemo
    main.cpp
    RenderGraphApp.h
    RenderGraphApp.cpp
    TrianglePass.h
    TrianglePass.cpp
)

target_link_libraries(RenderGraphDemo PRIVATE
    HappyCatEngine
    HappyCatRenderer
)

# Copy shaders
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../shaders/triangle
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)

set_target_properties(RenderGraphDemo PROPERTIES FOLDER "HappyCat/Samples")
