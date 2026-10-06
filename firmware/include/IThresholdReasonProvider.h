#pragma once

#include <cstdint>

namespace EnvNode {

// Evaluation reason, not a confirmation that an output command succeeded.
enum class ThresholdReason : uint8_t {
    NotStarted,
    Stopped,
    InvalidConfiguration,
    NoMeasurement,
    InvalidMeasurement,
    StaleMeasurement,
    OnThreshold,
    OffThreshold,
    HysteresisHold,
    AwaitingThreshold,
};

class IThresholdReasonProvider {
public:
    virtual ~IThresholdReasonProvider() = default;
    virtual ThresholdReason reason() const = 0;
};

} // namespace EnvNode
