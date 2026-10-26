#pragma once

#include <cstdint>

namespace EnvNode {

class ILogTimeProvider {
public:
    virtual ~ILogTimeProvider() = default;

    virtual uint32_t monotonicMs() const = 0;
    virtual bool wallClockEpochSeconds(int64_t& epochSeconds) const = 0;
};

} // namespace EnvNode
