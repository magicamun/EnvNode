#pragma once

#include <cstddef>

namespace EnvNode {

class ISensorRuntime {
public:
    virtual ~ISensorRuntime() = default;
    virtual bool rebuild(size_t& activeSensorCount, const char*& failureReason) = 0;
};

} // namespace EnvNode
