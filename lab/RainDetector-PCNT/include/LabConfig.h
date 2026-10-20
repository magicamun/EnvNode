#pragma once

#include <stdint.h>

namespace LabConfig {
constexpr int RainFreqGpio = 27;
constexpr int HeaterGpio = 16;
constexpr unsigned HeaterPwmChannel = 0;
constexpr unsigned HeaterPwmFrequencyHz = 5000;
constexpr unsigned HeaterPwmResolutionBits = 8;
constexpr unsigned HeaterMaxDurationSeconds = 120;
static_assert(HeaterGpio != RainFreqGpio, "Heater and frequency input must be separate");
constexpr int64_t MeasurementWindowUs = 250000;
constexpr unsigned long SerialBaud = 115200;
}  // namespace LabConfig
