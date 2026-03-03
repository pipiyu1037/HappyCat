# HappyCat RHI Module

add_library(HappyCatRHI STATIC
    # Common
    RHITypes.h
    RHIDefs.h

    # Vulkan Implementation
    Vulkan/VKCommon.h
    Vulkan/VKInstance.h
    Vulkan/VKInstance.cpp
    Vulkan/VKPhysicalDevice.h
    Vulkan/VKPhysicalDevice.cpp
    Vulkan/VKDevice.h
    Vulkan/VKDevice.cpp
    Vulkan/VKQueue.h
    Vulkan/VKQueue.cpp
    Vulkan/VKCommandPool.h
    Vulkan/VKCommandPool.cpp
    Vulkan/VKCommandBuffer.h
    Vulkan/VKCommandBuffer.cpp
    Vulkan/VKSwapChain.h
    Vulkan/VKSwapChain.cpp
    Vulkan/VKSemaphore.h
    Vulkan/VKSemaphore.cpp
    Vulkan/VKFence.h
    Vulkan/VKFence.cpp
    Vulkan/VKBuffer.h
    Vulkan/VKBuffer.cpp
    Vulkan/VKImage.h
    Vulkan/VKImage.cpp
    Vulkan/VKImageView.h
    Vulkan/VKImageView.cpp
    Vulkan/VKShaderModule.h
    Vulkan/VKShaderModule.cpp
    Vulkan/VKPipeline.h
    Vulkan/VKPipeline.cpp
    Vulkan/VKRenderPass.h
    Vulkan/VKRenderPass.cpp
    Vulkan/VKFramebuffer.h
    Vulkan/VKFramebuffer.cpp
)

target_include_directories(HappyCatRHI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HappyCatRHI PUBLIC
    HappyCatCore
    Vulkan::Vulkan
)

# Vulkan validation
if(HAPPYCAT_ENABLE_VULKAN_VALIDATION)
    target_compile_definitions(HappyCatRHI PUBLIC HC_VULKAN_VALIDATION=1)
endif()

set_target_properties(HappyCatRHI PROPERTIES FOLDER "HappyCat/Modules")
