# HappyCat Renderer Module
cmake_minimum_required(VERSION 3.20)
project(HappyCatRenderer
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "HappyCat Renderer Module"
)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Source files
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

    # Shader
    Shader/ShaderCompiler.h
    Shader/ShaderCompiler.cpp
)

target_include_directories(HappyCatRenderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HappyCatRenderer PUBLIC
    HappyCatCore
    HappyCatRHI

    # Link glslang libraries from Vulkan SDK using absolute paths
    "${VULKAN_SDK}/Lib/glslang.lib"
    "${VULKAN_SDK}/Lib/SPIRV.lib"
    "${VULKAN_SDK}/Lib/SPIRV-Tools.lib"
    "${VULKAN_SDK}/Lib/SPIRV-Tools-opt.lib"
    "${VULKAN_SDK}/Lib/glslang-default-resource-limits.lib"
)

set_target_properties(HappyCatRenderer PROPERTIES FOLDER "HappyCat/Modules")
