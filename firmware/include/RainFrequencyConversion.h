// EnvNode-owned: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include "IRainFrequencyInput.h"

namespace EnvNode {
// Pure conversion used by PCNT and host tests. Do not interpret residual counts
// after a wrap as a frequency. Large service delays are measurement gaps.
inline RainFrequencyWindow rainFrequencyWindow(uint64_t startUs, uint64_t endUs,
    uint32_t pulses, bool overflow) {
    RainFrequencyWindow result;
    result.startUs = startUs;
    result.endUs = endUs;
    result.pulseCount = pulses;
    if (endUs <= startUs || endUs - startUs < 100000 || endUs - startUs > 1500000)
        result.fault = RainFrequencyFault::InvalidWindow;
    else if (overflow) result.fault = RainFrequencyFault::CounterOverflow;
    else if (!pulses) result.fault = RainFrequencyFault::NoSignal;
    else {
        result.frequencyHz = static_cast<float>(pulses * 1000000.0 / (endUs - startUs));
        result.fault = RainFrequencyFault::None;
    }
    return result;
}
} // namespace EnvNode
