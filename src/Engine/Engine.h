#pragma once

#include "Core/Utils/Types.h"

namespace happycat {

class Engine {
public:
    static void Initialize();
    static void Shutdown();

private:
    Engine() = delete;
};

} // namespace happycat
