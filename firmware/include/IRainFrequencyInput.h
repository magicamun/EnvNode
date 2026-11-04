// EnvNode-owned: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include <cstdint>

namespace EnvNode {
enum class RainFrequencyFault : uint8_t {
    None, NoSignal, CounterOverflow, InvalidWindow, DriverError
};
inline const char* rainFrequencyFaultName(RainFrequencyFault fault) {
    switch (fault) {
        case RainFrequencyFault::None: return "none";
        case RainFrequencyFault::NoSignal: return "no_signal";
        case RainFrequencyFault::CounterOverflow: return "counter_overflow";
        case RainFrequencyFault::InvalidWindow: return "invalid_window";
        default: return "driver_error";
    }
}
struct RainFrequencyWindow {
    uint64_t startUs = 0;
    uint64_t endUs = 0;
    uint32_t pulseCount = 0;
    float frequencyHz = 0;
    RainFrequencyFault fault = RainFrequencyFault::DriverError;
    bool valid() const { return fault == RainFrequencyFault::None; }
};
enum class RainFrequencyPoll { Pending, Window, Failed };

// Acquisition boundary shared by physical and simulated inputs. A Window may
// contain invalid physical data (zero signal, overflow, unsuitable gate time).
// Failed means acquisition itself failed; Pending never blocks the caller.
class IRainFrequencyInput {
public:
    virtual ~IRainFrequencyInput() = default;
    virtual bool begin(uint8_t gpio) = 0;
    virtual RainFrequencyPoll poll(RainFrequencyWindow& result) = 0;
    virtual void stop() = 0;
};
} // namespace EnvNode
