#include "TexturedQuadApp.h"
#include "Core/Utils/Logger.h"

int main() {
    happycat::Logger::Initialize();

    try {
        happycat::ApplicationConfig config;
        config.name = "HappyCat - Textured Quad Demo";
        config.windowWidth = 1280;
        config.windowHeight = 720;
        config.enableValidation = true;
        config.framesInFlight = 2;

        happycat::TexturedQuadApp app(config);
        app.Run();
    } catch (const std::exception& e) {
        HC_CORE_CRITICAL("Application failed: {0}", e.what());
        happycat::Logger::Shutdown();
        return -1;
    }

    happycat::Logger::Shutdown();
    return 0;
}
