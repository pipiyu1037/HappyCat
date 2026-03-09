# HappyCat Engine - Third Party Dependencies Configuration
set(HAPPYCAT_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

find_package(Vulkan REQUIRED)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${HAPPYCAT_THIRD_PARTY_DIR}/glfw)

# ImGui
add_library(imgui STATIC
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/imgui.cpp
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/imgui_demo.cpp
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${HAPPYCAT_THIRD_PARTY_DIR}/imgui ${HAPPYCAT_THIRD_PARTY_DIR}/imgui/backends)
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

# GLM
add_subdirectory(${HAPPYCAT_THIRD_PARTY_DIR}/glm)

# spdlog
add_subdirectory(${HAPPYCAT_THIRD_PARTY_DIR}/spdlog)

# stb (header-only)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${HAPPYCAT_THIRD_PARTY_DIR}/stb)

# tinyobjloader
add_subdirectory(${HAPPYCAT_THIRD_PARTY_DIR}/tinyobjloader)

# tinygltf (header-only)
add_library(tinygltf INTERFACE)
target_include_directories(tinygltf INTERFACE ${HAPPYCAT_THIRD_PARTY_DIR}/tinygltf)

# glslang and SPIRV-Cross from Vulkan SDK
# NOTE: These are disabled because:
# 1. Vulkan SDK 1.4.341.1 has a CMake config bug for SPIRV-Tools
# 2. Shaders are precompiled (.spv files), so runtime compilation is not needed
# 3. No source code currently uses these libraries
#
# To re-enable when needed:
# find_package(glslang REQUIRED PATHS "${VULKAN_SDK}/Lib/cmake/glslang/glslang" NO_DEFAULT_PATH)
# find_package(spirv_cross_core REQUIRED PATHS "${VULKAN_SDK}/Lib/cmake/spirv_cross_core" NO_DEFAULT_PATH)
# find_package(spirv_cross_glsl REQUIRED PATHS "${VULKAN_SDK}/Lib/cmake/spirv_cross_glsl" NO_DEFAULT_PATH)
# find_package(spirv_cross_reflect REQUIRED PATHS "${VULKAN_SDK}/Lib/cmake/spirv_cross_reflect" NO_DEFAULT_PATH)

# Interface library (for future use when shader compilation is needed)
add_library(HappyCatDeps INTERFACE)
target_link_libraries(HappyCatDeps INTERFACE
    glfw
    imgui
    glm::glm
    spdlog::spdlog
    stb
    tinyobjloader
    tinygltf
    Vulkan::Vulkan
    # Uncomment when shader compilation libraries are needed:
    # glslang::SPIRV glslang::glslang glslang::MachineIndependent glslang::GenericCodeGen glslang::OSDependent
    # spirv-cross-core spirv-cross-glsl spirv-cross-reflect
)
