#include "AverageAnalogSampler.h"

#include <cmath>
#include <limits>

namespace EnvNode {

AverageAnalogSampler::AverageAnalogSampler(
    IAnalogInput& input,
    IMonotonicClock& clock,
    const AnalogSamplingConfiguration& configuration)
    : input_(input)
    , clock_(clock)
    , configuration_(configuration) {
}

bool AverageAnalogSampler::begin() {
    initialized_ = false;
    hasCompletedWindow_ = false;
    latestCompletedWindow_ = {};
    voltageSum_ = 0.0;
    acceptedSamples_ = 0;
    rejectedSamples_ = 0;
    if (!configurationValid() || !input_.begin() || !input_.initialized()) return false;

    const uint32_t nowMs = clock_.nowMs();
    windowStartMs_ = nowMs;
    windowDeadlineMs_ = nowMs + configuration_.filterWindowMs;
    sampleDeadlineMs_ = nowMs;
    initialized_ = true;
    return true;
}

void AverageAnalogSampler::service() {
    if (!initialized_) return;
    const uint32_t nowMs = clock_.nowMs();
    if (deadlineReached(nowMs, windowDeadlineMs_)) completeWindow(nowMs);
    if (deadlineReached(nowMs, sampleDeadlineMs_)) takeSample(nowMs);
}

bool AverageAnalogSampler::initialized() const {
    return initialized_;
}

bool AverageAnalogSampler::hasCompletedWindow() const {
    return hasCompletedWindow_;
}

const FilteredAnalogValue& AverageAnalogSampler::latestCompletedWindow() const {
    return latestCompletedWindow_;
}

bool AverageAnalogSampler::deadlineReached(uint32_t nowMs, uint32_t deadlineMs) {
    return static_cast<int32_t>(nowMs - deadlineMs) >= 0;
}

bool AverageAnalogSampler::configurationValid() const {
    const uint32_t maximumDeadlineDistance = static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
    return configuration_.sampleIntervalMs > 0
        && configuration_.sampleIntervalMs <= maximumDeadlineDistance
        && configuration_.filterWindowMs > 0
        && configuration_.filterWindowMs <= maximumDeadlineDistance
        && configuration_.minimumValidSamples > 0;
}

void AverageAnalogSampler::completeWindow(uint32_t nowMs) {
    latestCompletedWindow_.voltage = acceptedSamples_ > 0
        ? static_cast<float>(voltageSum_ / acceptedSamples_)
        : 0.0f;
    latestCompletedWindow_.windowStartMs = windowStartMs_;
    latestCompletedWindow_.windowEndMs = windowDeadlineMs_;
    latestCompletedWindow_.acceptedSamples = acceptedSamples_;
    latestCompletedWindow_.rejectedSamples = rejectedSamples_;
    latestCompletedWindow_.valid = acceptedSamples_ >= configuration_.minimumValidSamples;
    hasCompletedWindow_ = true;

    windowStartMs_ = nowMs;
    windowDeadlineMs_ = nowMs + configuration_.filterWindowMs;
    voltageSum_ = 0.0;
    acceptedSamples_ = 0;
    rejectedSamples_ = 0;
}

void AverageAnalogSampler::takeSample(uint32_t nowMs) {
    const AnalogSample sample = input_.read();
    if (sample.valid
        && sample.error == AnalogInputError::None
        && std::isfinite(sample.voltage)) {
        voltageSum_ += sample.voltage;
        ++acceptedSamples_;
    } else {
        ++rejectedSamples_;
    }
    sampleDeadlineMs_ = nowMs + configuration_.sampleIntervalMs;
}

} // namespace EnvNode
