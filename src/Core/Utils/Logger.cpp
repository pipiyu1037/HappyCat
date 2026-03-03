#include "Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace happycat {

Ref<spdlog::logger> Logger::s_CoreLogger;
Ref<spdlog::logger> Logger::s_ClientLogger;

void Logger::Initialize() {
    // Set pattern: [timestamp] [logger name] [level] message
    spdlog::set_pattern("%^[%T] %n [%l]: %v%$");

    // Core logger (for engine internals)
    s_CoreLogger = spdlog::stdout_color_mt("HAPPYCAT");
    s_CoreLogger->set_level(spdlog::level::trace);

    // Client logger (for application)
    s_ClientLogger = spdlog::stdout_color_mt("APP");
    s_ClientLogger->set_level(spdlog::level::trace);

    HC_CORE_INFO("Logger initialized");
}

void Logger::Shutdown() {
    HC_CORE_INFO("Logger shutdown");
    s_CoreLogger.reset();
    s_ClientLogger.reset();
    spdlog::shutdown();
}

} // namespace happycat
