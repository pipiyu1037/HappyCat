#include "Application.h"
#include "Platform/Window/Window.h"
#include "RHI/Vulkan/VKInstance.h"
#include "RHI/Vulkan/VKDevice.h"
#include "RHI/Vulkan/VKSwapChain.h"
#include "FrameContext.h"
#include "Core/Utils/Logger.h"

#include <chrono>

namespace happycat {

Application::Application(const ApplicationConfig& config) : m_Config(config) {
}

Application::~Application() {
    Shutdown();
}

bool Application::Initialize() {
    // Create window
    WindowCreateInfo windowInfo{};
    windowInfo.title = m_Config.name;
    windowInfo.width = m_Config.windowWidth;
    windowInfo.height = m_Config.windowHeight;
    m_Window.reset(Window::Create(windowInfo));

    if (!m_Window) {
        HC_CORE_CRITICAL("Failed to create window");
        return false;
    }

    // Create Vulkan instance
    VKInstance::CreateInfo instanceInfo{};
    instanceInfo.applicationName = m_Config.name;
    instanceInfo.enableValidation = m_Config.enableValidation;
    m_Instance = VKInstance::Create(instanceInfo);

    if (!m_Instance) {
        HC_CORE_CRITICAL("Failed to create Vulkan instance");
        return false;
    }

    // Create surface
    m_Surface = m_Window->CreateSurface(m_Instance->GetHandle());

    // Pick physical device
    VkPhysicalDevice physicalDevice = m_Instance->PickPhysicalDevice(m_Surface);

    // Create device
    VKDevice::CreateInfo deviceInfo{};
    deviceInfo.physicalDevice = physicalDevice;
    deviceInfo.surface = m_Surface;
    deviceInfo.enableValidation = m_Config.enableValidation;
    m_Device = VKDevice::Create(deviceInfo);

    if (!m_Device) {
        HC_CORE_CRITICAL("Failed to create Vulkan device");
        return false;
    }

    // Create swapchain
    VKSwapChain::CreateInfo swapChainInfo{};
    swapChainInfo.width = m_Config.windowWidth;
    swapChainInfo.height = m_Config.windowHeight;
    swapChainInfo.surface = m_Surface;
    swapChainInfo.vsync = true;
    m_SwapChain = VKSwapChain::Create(m_Device.get(), swapChainInfo);

    if (!m_SwapChain) {
        HC_CORE_CRITICAL("Failed to create swap chain");
        return false;
    }

    // Create frame context
    m_FrameContext = std::make_unique<FrameContext>(m_Device.get(), m_Config.framesInFlight,
                                                      m_SwapChain->GetImageCount());

    // Set window callbacks
    m_Window->SetResizeCallback([this](u32 width, u32 height) {
        // Handle window minimization
        if (width == 0 || height == 0) {
            m_Minimized = true;
            return;
        }
        m_Minimized = false;
        m_NeedResize = true;
        OnResize(width, height);
    });

    m_Window->SetCloseCallback([this]() {
        m_Running = false;
    });

    // Call derived class init
    if (!OnInit()) {
        HC_CORE_CRITICAL("Application initialization failed");
        return false;
    }

    m_Initialized = true;
    return true;
}

void Application::Shutdown() {
    if (!m_Initialized) return;

    OnShutdown();

    m_Device->WaitIdle();

    m_FrameContext.reset();
    m_SwapChain.reset();
    m_Device.reset();

    // Destroy surface before instance
    if (m_Surface != VK_NULL_HANDLE && m_Instance) {
        vkDestroySurfaceKHR(m_Instance->GetHandle(), m_Surface, nullptr);
        m_Surface = VK_NULL_HANDLE;
    }

    m_Instance.reset();
    m_Window.reset();

    m_Initialized = false;
}

void Application::Run() {
    if (!Initialize()) {
        return;
    }

    m_Running = true;

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (m_Running && !m_Window->ShouldClose()) {
        m_Window->PollEvents();

        // Skip rendering when minimized
        if (m_Minimized) {
            continue;
        }

        // Recreate swapchain if needed
        if (m_NeedResize) {
            m_Device->WaitIdle();
            u32 width, height;
            m_Window->GetFramebufferSize(width, height);
            m_SwapChain->Recreate(width, height);
            m_NeedResize = false;
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        f32 deltaTime = std::chrono::duration<f32, std::chrono::seconds::period>(
            currentTime - lastTime).count();
        lastTime = currentTime;

        OnUpdate(deltaTime);
        OnRender();
    }

    Shutdown();
}

} // namespace happycat
