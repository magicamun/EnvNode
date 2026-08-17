#pragma once

#include "ILogTimeProvider.h"
#include "IMonotonicClock.h"

namespace EnvNode {

class SystemLogTimeProvider : public ILogTimeProvider {
public:
    explicit SystemLogTimeProvider(IMonotonicClock& monotonicClock);

    uint32_t monotonicMs() const override;
    bool wallClockEpochSeconds(int64_t& epochSeconds) const override;

private:
    IMonotonicClock& monotonicClock_;
};

} // namespace EnvNode
