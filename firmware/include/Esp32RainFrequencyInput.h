// EnvNode-owned: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include "IRainFrequencyInput.h"

namespace EnvNode {
// Classic ESP32 PCNT, individually owned units. No GPIO polling, ISR, delay or
// heap allocation. A driver failure is isolated to this instance.
class Esp32RainFrequencyInput : public IRainFrequencyInput {
public:
    ~Esp32RainFrequencyInput() override;
    Esp32RainFrequencyInput() = default;
    Esp32RainFrequencyInput(const Esp32RainFrequencyInput&) = delete;
    Esp32RainFrequencyInput& operator=(const Esp32RainFrequencyInput&) = delete;
    bool begin(uint8_t gpio) override;
    RainFrequencyPoll poll(RainFrequencyWindow& result) override;
    void stop() override;
private:
    bool startWindow();
    int unit_ = -1;
    uint64_t startUs_ = 0;
    bool running_ = false;
    bool gateOpen_ = false;
};
} // namespace EnvNode
