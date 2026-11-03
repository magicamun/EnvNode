#include "RecentLogStore.h"
#include <new>

namespace EnvNode {

RecentLogStore::RecentLogStore(size_t capacity) {
    if (capacity > RecentLogCapacity) capacity = RecentLogCapacity;
    if (capacity == 0) return;
    entries_.reset(new (std::nothrow) LogEntry[capacity]{});
    if (entries_) capacity_ = capacity;
}

size_t RecentLogStore::count() const {
    return count_;
}

size_t RecentLogStore::capacity() const {
    return capacity_;
}

bool RecentLogStore::copyEntry(size_t logicalIndex, LogEntry& result) const {
    if (logicalIndex >= count_) return false;
    result = entries_[(oldestIndex_ + logicalIndex) % capacity_];
    return true;
}

void RecentLogStore::append(const LogEntry& entry) {
    if (!entries_) return;
    if (count_ < capacity_) {
        entries_[(oldestIndex_ + count_) % capacity_] = entry;
        ++count_;
        return;
    }
    entries_[oldestIndex_] = entry;
    oldestIndex_ = (oldestIndex_ + 1) % capacity_;
}

void RecentLogStore::clear() {
    oldestIndex_ = 0;
    count_ = 0;
}

} // namespace EnvNode
