#pragma once

#include "Core/Utils/Types.h"
#include "RHI/RHIDefs.h"
#include <memory>
#include <string>

namespace happycat {

class Window;
class VKInstance;
class VKDevice;
class VKSwapChain;
class FrameContext;

struct ApplicationConfig {
    std::string name = "HappyCat Application";
    u32 windowWidth = 1280;
    u32 windowHeight = 720;
    bool enableValidation = true;
    u32 framesInFlight = 2;
};

class Application {
public:
    explicit Application(const ApplicationConfig& config);
    virtual ~Application();

    // Non-copyable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Run the application
    void Run();

    // Close the application
    void Close() { m_Running = false; }

protected:
    // Override these in derived class
    virtual bool OnInit() = 0;
    virtual void OnShutdown() = 0;
    virtual void OnUpdate(f32 deltaTime) = 0;
    virtual void OnRender() = 0;
    virtual void OnResize(u32 width, u32 height) { }

    // Getters
    Window* GetWindow() const { return m_Window.get(); }
    VKDevice* GetDevice() const { return m_Device.get(); }
    VKSwapChain* GetSwapChain() const { return m_SwapChain.get(); }
    FrameContext* GetFrameContext() const { return m_FrameContext.get(); }

private:
    bool Initialize();
    void Shutdown();
    void MainLoop();

    ApplicationConfig m_Config;
    bool m_Running = false;
    bool m_Initialized = false;
    bool m_Minimized = false;
    bool m_NeedResize = false;

    std::unique_ptr<Window> m_Window;
    std::unique_ptr<VKInstance> m_Instance;
    std::unique_ptr<VKDevice> m_Device;
    std::unique_ptr<VKSwapChain> m_SwapChain;
    std::unique_ptr<FrameContext> m_FrameContext;

    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
};

} // namespace happycat
