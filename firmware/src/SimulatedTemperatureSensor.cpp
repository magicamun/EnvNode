#include "SimulatedTemperatureSensor.h"

#include <cmath>

namespace EnvNode {
namespace {

const float CenterTemperatureCelsius = 20.0F;
const float TemperatureAmplitudeCelsius = 5.0F;
const uint32_t WavePeriodMs = 120000;
const double TwoPi = 6.28318530717958647692;

} // namespace

SimulatedTemperatureSensor::SimulatedTemperatureSensor(
    SensorId id,
    IMonotonicClock& monotonicClock)
    : id_(id)
    , monotonicClock_(monotonicClock) {
}

SensorId SimulatedTemperatureSensor::id() const {
    return id_;
}

const char* SimulatedTemperatureSensor::type() const {
    return "Simulated Temperature";
}

SensorProvenance SimulatedTemperatureSensor::provenance() const {
    return SensorProvenance::Simulated;
}

SensorState SimulatedTemperatureSensor::state() const {
    return state_;
}

bool SimulatedTemperatureSensor::supports(MeasurementType type) const {
    return type == MeasurementType::Temperature;
}

void SimulatedTemperatureSensor::begin() {
    state_ = SensorState::Initializing;
    lastMonotonicMs_ = monotonicClock_.nowMs();
    elapsedMs_ = 0;
    state_ = SensorState::Ready;
}

SensorOperationResult SimulatedTemperatureSensor::service(IMeasurementSink& output) {
    (void)output;
    return SensorOperationResult::NoData;
}

SensorOperationResult SimulatedTemperatureSensor::sample(IMeasurementSink& output) {
    if (state_ != SensorState::Ready && state_ != SensorState::Degraded) {
        return SensorOperationResult::NoData;
    }

    const uint32_t nowMs = monotonicClock_.nowMs();
    elapsedMs_ += static_cast<uint32_t>(nowMs - lastMonotonicMs_);
    lastMonotonicMs_ = nowMs;

    const uint64_t phaseMs = elapsedMs_ % WavePeriodMs;
    const double phase = TwoPi * static_cast<double>(phaseMs) / WavePeriodMs;
    const float temperature = CenterTemperatureCelsius
        + TemperatureAmplitudeCelsius * static_cast<float>(std::sin(phase));

    Measurement measurement;
    measurement.type = MeasurementType::Temperature;
    measurement.value = MeasurementValue::floatingPoint(temperature);
    measurement.valid = true;
    measurement.quality = MeasurementQuality::Good;

    output.emit(measurement);
    return SensorOperationResult::Completed;
}

} // namespace EnvNode
