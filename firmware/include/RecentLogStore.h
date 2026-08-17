#pragma once

#include <cstddef>

#include "LogEntry.h"

namespace EnvNode {

constexpr size_t RecentLogCapacity = 64;

class RecentLogStore {
public:
    size_t count() const;
    bool copyEntry(size_t logicalIndex, LogEntry& result) const;
    void append(const LogEntry& entry);
    void clear();

private:
    LogEntry entries_[RecentLogCapacity] = {};
    size_t oldestIndex_ = 0;
    size_t count_ = 0;
};

} // namespace EnvNode
