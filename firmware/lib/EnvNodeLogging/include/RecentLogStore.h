#pragma once

#include <cstddef>
#include <memory>

#include "IRecentLogReader.h"

namespace EnvNode {

constexpr size_t RecentLogCapacity = 64;

class RecentLogStore : public IRecentLogReader {
public:
    // Fixed retention limit, allocated once on the heap to leave static DRAM
    // available for optional drivers. Allocation failure disables retention only.
    explicit RecentLogStore(size_t capacity = RecentLogCapacity);
    size_t count() const override;
    size_t capacity() const override;
    bool copyEntry(size_t logicalIndex, LogEntry& result) const override;
    void append(const LogEntry& entry);
    void clear();

private:
    std::unique_ptr<LogEntry[]> entries_;
    size_t capacity_ = 0;
    size_t oldestIndex_ = 0;
    size_t count_ = 0;
};

} // namespace EnvNode
