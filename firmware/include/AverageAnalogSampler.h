#pragma once

#include <cstddef>
#include <cstdint>

#include "IAnalogInput.h"
#include "IMonotonicClock.h"

namespace EnvNode {

struct AnalogSamplingConfiguration {
    uint32_t sampleIntervalMs;
    uint32_t filterWindowMs;
    size_t minimumValidSamples;
};

struct FilteredAnalogValue {
    float voltage = 0.0f;
    uint32_t windowStartMs = 0;
    uint32_t windowEndMs = 0;
    size_t acceptedSamples = 0;
    size_t rejectedSamples = 0;
    bool valid = false;
};

class AverageAnalogSampler {
public:
    AverageAnalogSampler(
        IAnalogInput& input,
        IMonotonicClock& clock,
        const AnalogSamplingConfiguration& configuration);

    bool begin();
    void service();
    bool initialized() const;
    bool hasCompletedWindow() const;
    const FilteredAnalogValue& latestCompletedWindow() const;

private:
    static bool deadlineReached(uint32_t nowMs, uint32_t deadlineMs);
    bool configurationValid() const;
    void completeWindow(uint32_t nowMs);
    void takeSample(uint32_t nowMs);

    IAnalogInput& input_;
    IMonotonicClock& clock_;
    AnalogSamplingConfiguration configuration_;
    uint32_t windowStartMs_ = 0;
    uint32_t windowDeadlineMs_ = 0;
    uint32_t sampleDeadlineMs_ = 0;
    double voltageSum_ = 0.0;
    size_t acceptedSamples_ = 0;
    size_t rejectedSamples_ = 0;
    FilteredAnalogValue latestCompletedWindow_;
    bool initialized_ = false;
    bool hasCompletedWindow_ = false;
};

} // namespace EnvNode
