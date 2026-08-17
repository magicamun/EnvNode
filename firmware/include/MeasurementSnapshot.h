#pragma once

#include <cstdint>

#include "Measurement.h"

namespace EnvNode {

struct MeasurementSnapshot {
    Measurement measurement;
    // Monotonic acceptance time is used for age/freshness calculations.
    uint32_t acceptedMonotonicMs = 0;
    // Revision identifies a stored update. Consumers use equality only.
    uint32_t revision = 0;
};

} // namespace EnvNode
