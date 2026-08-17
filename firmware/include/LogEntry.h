#pragma once

#include <cstddef>
#include <cstdint>

#include "LogLevel.h"

namespace EnvNode {

constexpr size_t LogMessageCapacity = 256;

struct LogEntry {
    uint32_t sequence = 0;
    uint32_t monotonicMs = 0;
    int64_t epochSeconds = 0;
    LogLevel level = LogLevel::Info;
    bool wallClockValid = false;
    char message[LogMessageCapacity] = {};
};

} // namespace EnvNode
