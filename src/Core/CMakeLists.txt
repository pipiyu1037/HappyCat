# HappyCat Core Module

add_library(HappyCatCore STATIC
    # Utils
    Utils/Types.h
    Utils/Assert.h
    Utils/Logger.h
    Utils/Logger.cpp
    Utils/ResourceTracker.h
    Utils/ResourceTracker.cpp

    # Math
    Math/MathTypes.h

    # Threading
    Threading/ThreadPool.h
    Threading/ThreadPool.cpp
)

target_include_directories(HappyCatCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(HappyCatCore PUBLIC
    glm::glm
    spdlog::spdlog
)

# Precompiled header
if(MSVC AND HAPPYCAT_USE_PCH)
    target_precompile_headers(HappyCatCore PRIVATE
        <memory>
        <vector>
        <string>
        <unordered_map>
        <functional>
        <thread>
        <mutex>
        <condition_variable>
    )
endif()

set_target_properties(HappyCatCore PROPERTIES FOLDER "HappyCat/Modules")
