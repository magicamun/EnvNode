#include "SimulatedHumiditySensor.h"

#include <cmath>

namespace WeatherStation {
namespace {

const float CenterHumidityPercent = 52.5F;
const float HumidityAmplitudePercent = 7.5F;
const uint32_t WavePeriodMs = 300000;
const double TwoPi = 6.28318530717958647692;

} // namespace

SimulatedHumiditySensor::SimulatedHumiditySensor(
    SensorId id,
    IMonotonicClock& monotonicClock)
    : id_(id)
    , monotonicClock_(monotonicClock) {
}

SensorId SimulatedHumiditySensor::id() const {
    return id_;
}

SensorProvenance SimulatedHumiditySensor::provenance() const {
    return SensorProvenance::Simulated;
}

SensorState SimulatedHumiditySensor::state() const {
    return state_;
}

bool SimulatedHumiditySensor::supports(MeasurementType type) const {
    return type == MeasurementType::RelativeHumidity;
}

void SimulatedHumiditySensor::begin() {
    state_ = SensorState::Initializing;
    lastMonotonicMs_ = monotonicClock_.nowMs();
    elapsedMs_ = 0;
    state_ = SensorState::Ready;
}

SensorOperationResult SimulatedHumiditySensor::service(IMeasurementSink& output) {
    (void)output;
    return SensorOperationResult::NoData;
}

SensorOperationResult SimulatedHumiditySensor::sample(IMeasurementSink& output) {
    if (state_ != SensorState::Ready && state_ != SensorState::Degraded) {
        return SensorOperationResult::NoData;
    }

    const uint32_t nowMs = monotonicClock_.nowMs();
    elapsedMs_ += static_cast<uint32_t>(nowMs - lastMonotonicMs_);
    lastMonotonicMs_ = nowMs;

    const uint64_t phaseMs = elapsedMs_ % WavePeriodMs;
    const double phase = TwoPi * static_cast<double>(phaseMs) / WavePeriodMs;
    const float humidity = CenterHumidityPercent
        + HumidityAmplitudePercent * static_cast<float>(std::sin(phase));

    Measurement measurement;
    measurement.type = MeasurementType::RelativeHumidity;
    measurement.value = MeasurementValue::floatingPoint(humidity);
    measurement.valid = true;
    measurement.quality = MeasurementQuality::Good;
    output.emit(measurement);
    return SensorOperationResult::Completed;
}

} // namespace WeatherStation
