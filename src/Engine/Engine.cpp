#include "Engine.h"
#include "Core/Utils/Logger.h"

namespace happycat {

void Engine::Initialize() {
    Logger::Initialize();
    HC_CORE_INFO("HappyCat Engine initialized");
}

void Engine::Shutdown() {
    HC_CORE_INFO("HappyCat Engine shutdown");
    Logger::Shutdown();
}

} // namespace happycat
