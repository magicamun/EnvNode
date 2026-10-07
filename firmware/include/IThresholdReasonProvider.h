#pragma once

#include <cstdint>

namespace EnvNode {

enum class ThresholdDecision : uint8_t { Unknown, On, Off };

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
    virtual ThresholdDecision decision() const = 0;
    // Whether the retained decision is backed by a usable input in the latest evaluation.
    virtual bool decisionCurrent() const = 0;
};

} // namespace EnvNode
