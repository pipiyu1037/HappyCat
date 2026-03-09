cmake_minimum_required(VERSION 3.20)

project(HappyCat
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "A modern Vulkan-based rendering engine"
)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(HAPPYCAT_BUILD_SAMPLES "Build sample applications" ON)
option(HAPPYCAT_ENABLE_VULKAN_VALIDATION "Enable Vulkan validation layers" ON)
option(HAPPYCAT_USE_PCH "Use precompiled headers" ON)

# Set Vulkan SDK path if not set
if(NOT DEFINED VULKAN_SDK)
    set(VULKAN_SDK "C:/VulkanSDK/1.4.341.1")
endif()

# Find Vulkan manually
set(Vulkan_INCLUDE_DIR "${VULKAN_SDK}/Include")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(Vulkan_LIBRARY "${VULKAN_SDK}/Lib/vulkan-1.lib")
else()
    set(Vulkan_LIBRARY "${VULKAN_SDK}/Lib32/vulkan-1.lib")
endif()

# Create Vulkan imported target
if(NOT TARGET Vulkan::Vulkan)
    add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
    set_target_properties(Vulkan::Vulkan PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIR}"
        IMPORTED_LOCATION "${Vulkan_LIBRARY}"
    )
endif()

message(STATUS "Vulkan SDK: ${VULKAN_SDK}")
message(STATUS "Vulkan Include: ${Vulkan_INCLUDE_DIR}")
message(STATUS "Vulkan Library: ${Vulkan_LIBRARY}")

# Set CMAKE_PREFIX_PATH to find packages in Vulkan SDK
list(APPEND CMAKE_PREFIX_PATH "${VULKAN_SDK}/Lib/cmake")

# Third-party dependencies
include(${CMAKE_CURRENT_SOURCE_DIR}/third_party/HappyCatDeps.cmake)

# Source directories
add_subdirectory(src)

# Samples
if(HAPPYCAT_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()

# Copy shaders to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# IDE settings
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Organize targets in IDE
set_target_properties(HappyCatCore HappyCatRHI HappyCatRenderer HappyCatPlatform HappyCatEngine
    PROPERTIES FOLDER "HappyCat/Modules"
)
