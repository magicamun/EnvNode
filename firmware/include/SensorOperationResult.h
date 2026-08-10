#pragma once

namespace EnvNode {

enum class SensorOperationResult {
    // The operation succeeded. sample() normally emitted at least one Measurement.
    Completed,
    // The operation succeeded without emitting a Measurement.
    NoData,
    // The operation failed. Any Measurements emitted before the failure remain valid.
    HardwareFailure,
};

} // namespace EnvNode
