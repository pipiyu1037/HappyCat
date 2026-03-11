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

# Use shaderc shared library (DLL) to avoid MSVC version incompatibility issues
# with static libraries from Vulkan SDK
target_link_libraries(HappyCatRenderer PUBLIC
    HappyCatCore
    HappyCatRHI
    "${VULKAN_SDK}/Lib/shaderc_shared.lib"
)

# Copy shaderc DLL to output directory
if(EXISTS "${VULKAN_SDK}/Bin/shaderc_shared.dll")
    add_custom_command(TARGET HappyCatRenderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${VULKAN_SDK}/Bin/shaderc_shared.dll"
        "$<TARGET_FILE_DIR:HappyCatRenderer>"
    )
endif()

set_target_properties(HappyCatRenderer PROPERTIES FOLDER "HappyCat/Modules")
