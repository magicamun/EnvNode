#pragma once

#include <cstddef>

#include "LogEntry.h"

namespace EnvNode {

bool formatLogEntryTimestamp(
    const LogEntry& entry,
    char* output,
    size_t outputSize);

} // namespace EnvNode
