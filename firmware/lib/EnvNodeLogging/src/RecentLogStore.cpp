#include "RecentLogStore.h"

namespace EnvNode {

size_t RecentLogStore::count() const {
    return count_;
}

size_t RecentLogStore::capacity() const {
    return RecentLogCapacity;
}

bool RecentLogStore::copyEntry(size_t logicalIndex, LogEntry& result) const {
    if (logicalIndex >= count_) return false;
    result = entries_[(oldestIndex_ + logicalIndex) % RecentLogCapacity];
    return true;
}

void RecentLogStore::append(const LogEntry& entry) {
    if (count_ < RecentLogCapacity) {
        entries_[(oldestIndex_ + count_) % RecentLogCapacity] = entry;
        ++count_;
        return;
    }
    entries_[oldestIndex_] = entry;
    oldestIndex_ = (oldestIndex_ + 1) % RecentLogCapacity;
}

void RecentLogStore::clear() {
    oldestIndex_ = 0;
    count_ = 0;
}

} // namespace EnvNode
