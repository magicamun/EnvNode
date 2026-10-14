// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include "SeaLevelPressureSensor.h"
#include <cmath>
#include <climits>
#include <limits>
namespace EnvNode {
bool SeaLevelPressureSensor::supports(MeasurementType type) const {
    return sensor_->supports(type) || (type == MeasurementType::SeaLevelPressure
        && sensor_->supports(MeasurementType::AtmosphericPressure));
}
SensorOperationResult SeaLevelPressureSensor::service(IMeasurementSink& output) {
    output_ = &output; const auto result = sensor_->service(*this); output_ = nullptr; return result;
}
SensorOperationResult SeaLevelPressureSensor::sample(IMeasurementSink& output) {
    output_ = &output; const auto result = sensor_->sample(*this); output_ = nullptr; return result;
}
void SeaLevelPressureSensor::emit(const Measurement& input) {
    output_->emit(input);
    if (input.type != MeasurementType::AtmosphericPressure
        || !isMeasurementContentStructurallyValid(input)) return;
    Measurement derived = input;
    derived.type = MeasurementType::SeaLevelPressure;
    derived.valid = false;
    derived.value = MeasurementValue::none();
    derived.quality = MeasurementQuality::Degraded;
    status_ = PressureReductionStatus{};
    status_.temperatureSensorId = configuration_ ? configuration_->temperatureSensorId : InvalidSensorId;
    float pressure = 0;
    if (!position_->altitude.configured || !validGeoPosition(*position_)) {
        status_.reason = PressureReductionReason::NoAltitude;
    } else if (!input.valid || !input.value.tryGetFloatingPoint(pressure)
        || !std::isfinite(pressure) || pressure <= 0) {
        status_.reason = PressureReductionReason::InvalidPressure;
    } else {
        const double height = position_->altitude.value;
        double reduced = pressure / std::pow(1.0 - height / 44330.0, 5.255);
        status_.method = PressureReductionMethod::StandardAtmosphere;
        status_.reason = PressureReductionReason::TemperatureNotSelected;
        bool goodTemperature = false;
        if (configuration_ && configuration_->temperatureSensorId != InvalidSensorId) {
            MeasurementSnapshot temperature;
            float celsius = 0;
            if (!measurements_ || !clock_ || !measurements_->latest(
                    MeasurementSourceReference(configuration_->temperatureSensorId, MeasurementType::Temperature), temperature)) {
                status_.reason = PressureReductionReason::TemperatureUnavailable;
            } else {
                status_.hasTemperatureAge = true;
                status_.temperatureAgeMs = clock_->nowMs() - temperature.acceptedMonotonicMs;
                if (temperature.measurement.source != configuration_->temperatureSensorId
                    || temperature.measurement.type != MeasurementType::Temperature
                    || !temperature.measurement.valid
                    || !temperature.measurement.value.tryGetFloatingPoint(celsius)
                    || !std::isfinite(celsius) || celsius <= -273.15F
                    || (celsius + 273.15 + 0.0065 * height) <= 0) {
                    status_.reason = PressureReductionReason::TemperatureInvalid;
                } else if (configuration_->maxTemperatureAgeMs == 0
                    || configuration_->maxTemperatureAgeMs > INT32_MAX
                    || status_.temperatureAgeMs > configuration_->maxTemperatureAgeMs) {
                    status_.reason = PressureReductionReason::TemperatureStale;
                } else {
                    // Dry air with a constant 0.0065 K/m lapse rate; station temperature in kelvin.
                    const double kelvin = celsius + 273.15;
                    const double candidate = pressure * std::pow(
                        1.0 + 0.0065 * height / kelvin, 9.80665 / (287.05 * 0.0065));
                    if (!std::isfinite(candidate) || candidate <= 0
                        || candidate > std::numeric_limits<float>::max()) {
                        status_.reason = PressureReductionReason::TemperatureInvalid;
                    } else {
                        reduced = candidate;
                        status_.method = PressureReductionMethod::LocalTemperature;
                        goodTemperature = temperature.measurement.quality == MeasurementQuality::Good;
                        status_.reason = goodTemperature ? PressureReductionReason::None
                            : PressureReductionReason::TemperatureQuality;
                        if (temperature.measurement.provenance == SensorProvenance::Simulated)
                            derived.provenance = SensorProvenance::Simulated;
                    }
                }
            }
        }
        if (std::isfinite(reduced) && reduced > 0 && reduced <= std::numeric_limits<float>::max()) {
            derived.valid = true;
            derived.value = MeasurementValue::floatingPoint(static_cast<float>(reduced));
            derived.quality = input.quality == MeasurementQuality::Good && !goodTemperature
                ? MeasurementQuality::Estimated : input.quality;
        }
    }
    output_->emit(derived);
}
}
