#pragma once

#include <cstdint>

namespace EnvNode {

enum class LogLevel : uint8_t {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3,
};

const char* logLevelDisplayName(LogLevel level);

} // namespace EnvNode
