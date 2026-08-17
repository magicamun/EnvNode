#include "SystemLogTimeProvider.h"

#include <ctime>

namespace EnvNode {
namespace {

constexpr int64_t MinimumValidEpochSeconds = 1609459200LL;

} // namespace

SystemLogTimeProvider::SystemLogTimeProvider(IMonotonicClock& monotonicClock)
    : monotonicClock_(monotonicClock) {
}

uint32_t SystemLogTimeProvider::monotonicMs() const {
    return monotonicClock_.nowMs();
}

bool SystemLogTimeProvider::wallClockEpochSeconds(int64_t& epochSeconds) const {
    const time_t now = time(nullptr);
    if (static_cast<int64_t>(now) <= MinimumValidEpochSeconds) return false;
    epochSeconds = static_cast<int64_t>(now);
    return true;
}

} // namespace EnvNode
