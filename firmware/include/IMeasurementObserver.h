#pragma once

#include <cstdint>

#include "Measurement.h"

namespace EnvNode {

class IMeasurementObserver {
public:
    virtual ~IMeasurementObserver() = default;
    virtual void observe(const Measurement& measurement, uint32_t acceptedMonotonicMs) = 0;
    virtual void clear() = 0;
};

} // namespace EnvNode
