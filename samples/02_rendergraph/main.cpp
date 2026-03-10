#include "RenderGraphApp.h"
#include "Core/Utils/Logger.h"

int main() {
    happycat::Logger::Initialize();

    try {
        happycat::ApplicationConfig config{};
        config.name = "Render Graph Demo";
        config.windowWidth = 1280;
        config.windowHeight = 720;

        auto app = std::make_unique<happycat::RenderGraphApp>(config);
        app->Run();
    } catch (const std::exception& e) {
        HC_CORE_ERROR("Application error: {0}", e.what());
        happycat::Logger::Shutdown();
        return -1;
    }

    happycat::Logger::Shutdown();
    return 0;
}
