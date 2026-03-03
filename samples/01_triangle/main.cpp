#include "TriangleApp.h"
#include "Core/Utils/Logger.h"

int main() {
    happycat::Logger::Initialize();

    try {
        happycat::ApplicationConfig config;
        config.name = "HappyCat - Triangle Demo";
        config.windowWidth = 1280;
        config.windowHeight = 720;
        config.enableValidation = true;
        config.framesInFlight = 2;

        happycat::TriangleApp app(config);
        app.Run();
    } catch (const std::exception& e) {
        HC_CORE_CRITICAL("Application failed: {0}", e.what());
        happycat::Logger::Shutdown();
        return -1;
    }

    happycat::Logger::Shutdown();
    return 0;
}
