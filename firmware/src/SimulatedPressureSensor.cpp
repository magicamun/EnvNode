#include "SimulatedPressureSensor.h"

#include <cmath>

namespace WeatherStation {
namespace {

const float CenterPressurePascal = 101000.0F;
const float PressureAmplitudePascal = 500.0F;
const float CenterTemperatureCelsius = 22.0F;
const float TemperatureAmplitudeCelsius = 4.0F;
const uint32_t PressureWavePeriodMs = 900000;
const uint32_t TemperatureWavePeriodMs = 360000;
const double TwoPi = 6.28318530717958647692;

float sineValue(uint64_t elapsedMs, uint32_t periodMs) {
    const uint64_t phaseMs = elapsedMs % periodMs;
    const double phase = TwoPi * static_cast<double>(phaseMs) / periodMs;
    return static_cast<float>(std::sin(phase));
}

} // namespace

SimulatedPressureSensor::SimulatedPressureSensor(
    SensorId id,
    IMonotonicClock& monotonicClock)
    : id_(id)
    , monotonicClock_(monotonicClock) {
}

SensorId SimulatedPressureSensor::id() const {
    return id_;
}

const char* SimulatedPressureSensor::type() const {
    return "Simulated Pressure";
}

SensorProvenance SimulatedPressureSensor::provenance() const {
    return SensorProvenance::Simulated;
}

SensorState SimulatedPressureSensor::state() const {
    return state_;
}

bool SimulatedPressureSensor::supports(MeasurementType type) const {
    return type == MeasurementType::AtmosphericPressure
        || type == MeasurementType::Temperature;
}

void SimulatedPressureSensor::begin() {
    state_ = SensorState::Initializing;
    lastMonotonicMs_ = monotonicClock_.nowMs();
    elapsedMs_ = 0;
    state_ = SensorState::Ready;
}

SensorOperationResult SimulatedPressureSensor::service(IMeasurementSink& output) {
    (void)output;
    return SensorOperationResult::NoData;
}

SensorOperationResult SimulatedPressureSensor::sample(IMeasurementSink& output) {
    if (state_ != SensorState::Ready && state_ != SensorState::Degraded) {
        return SensorOperationResult::NoData;
    }

    const uint32_t nowMs = monotonicClock_.nowMs();
    elapsedMs_ += static_cast<uint32_t>(nowMs - lastMonotonicMs_);
    lastMonotonicMs_ = nowMs;

    Measurement pressureMeasurement;
    pressureMeasurement.type = MeasurementType::AtmosphericPressure;
    pressureMeasurement.value = MeasurementValue::floatingPoint(
        CenterPressurePascal
        + PressureAmplitudePascal * sineValue(elapsedMs_, PressureWavePeriodMs));
    pressureMeasurement.valid = true;
    pressureMeasurement.quality = MeasurementQuality::Good;
    output.emit(pressureMeasurement);

    Measurement temperatureMeasurement;
    temperatureMeasurement.type = MeasurementType::Temperature;
    temperatureMeasurement.value = MeasurementValue::floatingPoint(
        CenterTemperatureCelsius
        + TemperatureAmplitudeCelsius * sineValue(elapsedMs_, TemperatureWavePeriodMs));
    temperatureMeasurement.valid = true;
    temperatureMeasurement.quality = MeasurementQuality::Good;
    output.emit(temperatureMeasurement);

    return SensorOperationResult::Completed;
}

} // namespace WeatherStation
