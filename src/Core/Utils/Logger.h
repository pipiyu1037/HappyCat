#pragma once

#include "Types.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace happycat {

class Logger {
public:
    static void Initialize();
    static void Shutdown();

    static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
    static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

private:
    static Ref<spdlog::logger> s_CoreLogger;
    static Ref<spdlog::logger> s_ClientLogger;
};

} // namespace happycat

// Core log macros
#define HC_CORE_TRACE(...)    ::happycat::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#define HC_CORE_INFO(...)     ::happycat::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define HC_CORE_WARN(...)     ::happycat::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define HC_CORE_ERROR(...)    ::happycat::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define HC_CORE_CRITICAL(...) ::happycat::Logger::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define HC_TRACE(...)         ::happycat::Logger::GetClientLogger()->trace(__VA_ARGS__)
#define HC_INFO(...)          ::happycat::Logger::GetClientLogger()->info(__VA_ARGS__)
#define HC_WARN(...)          ::happycat::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define HC_ERROR(...)         ::happycat::Logger::GetClientLogger()->error(__VA_ARGS__)
#define HC_CRITICAL(...)      ::happycat::Logger::GetClientLogger()->critical(__VA_ARGS__)
