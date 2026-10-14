// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include "PressureReduction.h"
#include <climits>
#include "SensorSlotConfiguration.h"
namespace EnvNode {
bool eligiblePressureTemperatureSource(const SensorSlotConfiguration& slot) {
    if (!slot.enabled || !slot.schedule.enabled) return false;
    const auto* metadata = SensorImplementationRegistry::find(slot.implementation);
    if (!metadata) return false;
    for (size_t i = 0; i < metadata->measurementTypeCount; ++i)
        if (metadata->measurementTypes[i] == MeasurementType::Temperature) return true;
    return false;
}
bool validPressureReduction(const PressureReductionConfiguration& c, const SensorSlotConfiguration* slots) {
    if (c.maxTemperatureAgeMs == 0 || c.maxTemperatureAgeMs > INT32_MAX) return false;
    if (c.temperatureSensorId == InvalidSensorId) return true;
    for (size_t i = 0; i < MaxSensorSlotCount; ++i)
        if (slots[i].slotId == c.temperatureSensorId && eligiblePressureTemperatureSource(slots[i])) return true;
    return false;
}
bool parseTemperatureAgeSeconds(const String& text, uint32_t& milliseconds) {
    uint32_t whole = 0, fraction = 0; unsigned decimals = 0; bool dot = false, digit = false;
    for (size_t i = 0; i < text.length(); ++i) {
        const char c = text[i];
        if (c == '.' && !dot && digit) { dot = true; continue; }
        if (c < '0' || c > '9') return false;
        digit = true;
        if (dot) {
            if (++decimals > 3) return false;
            fraction = fraction * 10 + c - '0';
        } else {
            if (whole > (2147483U - (c - '0')) / 10) return false;
            whole = whole * 10 + c - '0';
        }
    }
    if (!digit || (dot && decimals == 0)) return false;
    while (decimals < 3) { fraction *= 10; ++decimals; }
    const uint32_t parsed = whole * 1000 + fraction;
    if (parsed == 0 || parsed > INT32_MAX) return false;
    milliseconds = parsed; return true;
}
String encodePressureReduction(const PressureReductionConfiguration& c) {
    return String(static_cast<unsigned int>(c.temperatureSensorId)) + ";"
        + String(static_cast<unsigned int>(c.maxTemperatureAgeMs));
}
bool decodePressureReduction(const String& text, PressureReductionConfiguration& configuration) {
    uint32_t values[2] = {}; size_t field = 0; bool digit = false;
    for (size_t i = 0; i < text.length(); ++i) {
        const char c = text[i];
        if (c == ';' && field == 0 && digit) { field = 1; digit = false; continue; }
        if (c < '0' || c > '9' || values[field] > (static_cast<uint32_t>(INT32_MAX) - (c - '0')) / 10) return false;
        values[field] = values[field] * 10 + c - '0'; digit = true;
    }
    if (field != 1 || !digit || values[0] > MaxSensorSlotCount || values[1] == 0) return false;
    configuration.temperatureSensorId = static_cast<SensorId>(values[0]);
    configuration.maxTemperatureAgeMs = values[1]; return true;
}

const char* pressureReductionMethodId(PressureReductionMethod method) {
    switch (method) {
        case PressureReductionMethod::StandardAtmosphere: return "standard_atmosphere";
        case PressureReductionMethod::LocalTemperature: return "local_temperature";
        default: return "none";
    }
}
const char* pressureReductionReasonId(PressureReductionReason reason) {
    switch (reason) {
        case PressureReductionReason::NoAltitude: return "no_altitude";
        case PressureReductionReason::InvalidPressure: return "invalid_pressure";
        case PressureReductionReason::TemperatureNotSelected: return "temperature_not_selected";
        case PressureReductionReason::TemperatureUnavailable: return "temperature_unavailable";
        case PressureReductionReason::TemperatureInvalid: return "temperature_invalid";
        case PressureReductionReason::TemperatureStale: return "temperature_stale";
        case PressureReductionReason::TemperatureQuality: return "temperature_quality";
        default: return "none";
    }
}
}
