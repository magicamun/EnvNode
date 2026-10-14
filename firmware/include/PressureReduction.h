// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#pragma once
#include "SensorId.h"
#include <Arduino.h>

namespace EnvNode {
struct SensorSlotConfiguration;
struct PressureReductionConfiguration {
    SensorId temperatureSensorId = InvalidSensorId;
    uint32_t maxTemperatureAgeMs = 60000;
};
enum class PressureReductionMethod : uint8_t { None, StandardAtmosphere, LocalTemperature };
enum class PressureReductionReason : uint8_t {
    None, NoAltitude, InvalidPressure, TemperatureNotSelected,
    TemperatureUnavailable, TemperatureInvalid, TemperatureStale, TemperatureQuality,
};
struct PressureReductionStatus {
    PressureReductionMethod method = PressureReductionMethod::None;
    PressureReductionReason reason = PressureReductionReason::None;
    SensorId temperatureSensorId = InvalidSensorId;
    bool hasTemperatureAge = false;
    uint32_t temperatureAgeMs = 0;
};
class IPressureReductionStatusResolver {
public:
    virtual ~IPressureReductionStatusResolver() = default;
    virtual bool pressureReductionStatus(SensorId id, PressureReductionStatus& status) const = 0;
};
bool eligiblePressureTemperatureSource(const SensorSlotConfiguration& slot);
bool validPressureReduction(const PressureReductionConfiguration& configuration, const SensorSlotConfiguration* slots);
bool parseTemperatureAgeSeconds(const String& text, uint32_t& milliseconds);
String encodePressureReduction(const PressureReductionConfiguration& configuration);
bool decodePressureReduction(const String& text, PressureReductionConfiguration& configuration);
const char* pressureReductionMethodId(PressureReductionMethod method);
const char* pressureReductionReasonId(PressureReductionReason reason);
} // namespace EnvNode
