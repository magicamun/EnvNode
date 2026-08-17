#pragma once

#include <cstddef>

#include "LogEntry.h"

namespace EnvNode {

class IRecentLogReader {
public:
    virtual ~IRecentLogReader() = default;

    virtual size_t count() const = 0;
    virtual size_t capacity() const = 0;
    virtual bool copyEntry(size_t logicalIndex, LogEntry& result) const = 0;
};

} // namespace EnvNode
