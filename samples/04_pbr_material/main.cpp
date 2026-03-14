#include "PBRMaterialApp.h"
#include "Core/Utils/Logger.h"

#include <iostream>

int main() {
    happycat::Logger::Initialize();

    std::cout << "Starting PBR Material Demo..." << std::endl;
    std::cout.flush();

    try {
        happycat::ApplicationConfig config{};
        config.name = "PBR Material Demo";
        config.windowWidth = 1280;
        config.windowHeight = 720;

        std::cout << "Creating application..." << std::endl;
        std::cout.flush();

        happycat::PBRMaterialApp app(config);

        std::cout << "Running application..." << std::endl;
        std::cout.flush();

        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        happycat::Logger::Shutdown();
        return 1;
    }

    happycat::Logger::Shutdown();
    return 0;
}
