# HappyCat Renderer Module

add_library(HappyCatRenderer STATIC
    # Render Graph
    RenderGraph/RenderResource.h
    RenderGraph/RenderResource.cpp
    RenderGraph/RenderPass.h
    RenderGraph/RenderPass.cpp
    RenderGraph/ResourceBuilder.h
    RenderGraph/DependencyBuilder.h
    RenderGraph/RenderGraphBuilder.h
    RenderGraph/RenderGraphBuilder.cpp
    RenderGraph/RenderPassContext.h
    RenderGraph/FrameContext.h
)

target_include_directories(HappyCatRenderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HappyCatRenderer PUBLIC
    HappyCatCore
    HappyCatRHI
)

set_target_properties(HappyCatRenderer PROPERTIES FOLDER "HappyCat/Modules")
