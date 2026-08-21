#include "AnalogPressureSensor.h"

#include <cmath>

namespace EnvNode {

AnalogPressureSensor::AnalogPressureSensor(SensorId id, AverageAnalogSampler& sampler,
    const AnalogPressureSensorConfiguration& configuration)
    : id_(id), sampler_(sampler), configuration_(configuration) {}

SensorId AnalogPressureSensor::id() const { return id_; }
const char* AnalogPressureSensor::type() const { return "Analog Pressure"; }
SensorProvenance AnalogPressureSensor::provenance() const { return SensorProvenance::Physical; }
SensorState AnalogPressureSensor::state() const { return state_; }

bool AnalogPressureSensor::supports(MeasurementType type) const {
    return type == MeasurementType::HydrostaticPressure
        || (configuration_.waterLevelEnabled && type == MeasurementType::WaterLevel);
}

void AnalogPressureSensor::begin() {
    state_ = SensorState::Initializing;
    if (!configurationValid() || !sampler_.begin()) {
        state_ = SensorState::Failed;
        return;
    }
    state_ = SensorState::Ready;
}

SensorOperationResult AnalogPressureSensor::service(IMeasurementSink& output) {
    (void)output;
    if (state_ != SensorState::Ready && state_ != SensorState::Degraded) {
        return SensorOperationResult::NoData;
    }
    sampler_.service();
    return SensorOperationResult::NoData;
}

SensorOperationResult AnalogPressureSensor::sample(IMeasurementSink& output) {
    if (state_ != SensorState::Ready && state_ != SensorState::Degraded) {
        return SensorOperationResult::NoData;
    }
    if (!sampler_.hasCompletedWindow()) return SensorOperationResult::NoData;

    const FilteredAnalogValue& window = sampler_.latestCompletedWindow();
    if (!window.valid
        || window.voltage < configuration_.validInputMinVoltage
        || window.voltage > configuration_.validInputMaxVoltage) {
        emitInvalid(output);
        return SensorOperationResult::Completed;
    }

    const LinearCalibrationResult pressure = applyLinearTwoPointCalibration(
        configuration_.pressureCalibration, window.voltage);
    if (!pressure.valid) {
        emitInvalid(output);
        return SensorOperationResult::Completed;
    }
    const MeasurementQuality quality = pressure.value < configuration_.minimumReliablePressurePascal
        ? MeasurementQuality::BelowMeasurementRange
        : MeasurementQuality::Good;
    float heightMetres = 0.0F;
    if (configuration_.waterLevelEnabled) {
        const double divisor = static_cast<double>(configuration_.waterLevel.liquidDensityKgPerCubicMetre)
            * configuration_.waterLevel.gravitationalAccelerationMetresPerSecondSquared;
        heightMetres = static_cast<float>(pressure.value / divisor);
        if (!std::isfinite(heightMetres)) {
            emitInvalid(output);
            return SensorOperationResult::Completed;
        }
    }
    emit(output, MeasurementType::HydrostaticPressure, pressure.value, quality);
    if (configuration_.waterLevelEnabled) {
        emit(output, MeasurementType::WaterLevel, heightMetres, quality);
    }
    return SensorOperationResult::Completed;
}

bool AnalogPressureSensor::configurationValid() const {
    return std::isfinite(configuration_.validInputMinVoltage)
        && std::isfinite(configuration_.validInputMaxVoltage)
        && configuration_.validInputMinVoltage < configuration_.validInputMaxVoltage
        && isValid(configuration_.pressureCalibration)
        && std::isfinite(configuration_.minimumReliablePressurePascal)
        && (!configuration_.waterLevelEnabled
            || (std::isfinite(configuration_.waterLevel.liquidDensityKgPerCubicMetre)
                && configuration_.waterLevel.liquidDensityKgPerCubicMetre > 0.0f
                && std::isfinite(configuration_.waterLevel.gravitationalAccelerationMetresPerSecondSquared)
                && configuration_.waterLevel.gravitationalAccelerationMetresPerSecondSquared > 0.0f));
}

void AnalogPressureSensor::emitInvalid(IMeasurementSink& output) const {
    Measurement pressure;
    pressure.type = MeasurementType::HydrostaticPressure;
    pressure.value = MeasurementValue::none();
    pressure.valid = false;
    pressure.quality = MeasurementQuality::Degraded;
    output.emit(pressure);
    if (configuration_.waterLevelEnabled) {
        pressure.type = MeasurementType::WaterLevel;
        output.emit(pressure);
    }
}

void AnalogPressureSensor::emit(IMeasurementSink& output, MeasurementType type,
    float value, MeasurementQuality quality) const {
    Measurement measurement;
    measurement.type = type;
    measurement.value = MeasurementValue::floatingPoint(value);
    measurement.valid = true;
    measurement.quality = quality;
    output.emit(measurement);
}

} // namespace EnvNode
