// EnvNode-owned: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include "RainDetectorSensor.h"
#include <cmath>
namespace EnvNode {
void RainDetectorSensor::begin() {
    observed_ = false;
    state_ = SensorState::Initializing;
    lastRetryMs_ = clock_.nowMs();
    if (!acquisition_.begin(gpio_)) state_ = SensorState::Failed;
}
void RainDetectorSensor::onRainFrequency(const RainFrequencyWindow& window) {
    latest_.window = window;
    latest_.acceptedMs = clock_.nowMs();
    ++latest_.revision;
    observed_ = true;
    state_ = window.valid() ? SensorState::Ready : SensorState::Degraded;
    const DiagnosticSource source(DiagnosticSourceKind::Sensor,id_);
    if (!diagnostics_.enabled(source)) return;
    DiagnosticEvent event("rain_window");
    event.integer("revision",latest_.revision);
    event.integer("start_us",static_cast<int64_t>(window.startUs));
    event.integer("end_us",static_cast<int64_t>(window.endUs));
    event.integer("pulses",window.pulseCount);
    if (std::isfinite(window.frequencyHz)) event.number("frequency_hz",window.frequencyHz);
    event.text("fault",rainFrequencyFaultName(window.fault));
    (void)diagnostics_.record(source,event);
}
SensorOperationResult RainDetectorSensor::service(IMeasurementSink& output) {
    (void)output;
    if (!acquisition_.running()) {
        if (static_cast<uint32_t>(clock_.nowMs()-lastRetryMs_) >= 1000) {
            lastRetryMs_ = clock_.nowMs();
            if (!acquisition_.begin(gpio_)) state_ = SensorState::Failed;
        }
        return SensorOperationResult::NoData;
    }
    acquisition_.service();
    if (!acquisition_.running()) {
        state_ = SensorState::Failed;
        lastRetryMs_ = clock_.nowMs();
        return SensorOperationResult::HardwareFailure;
    }
    return SensorOperationResult::NoData;
}
bool RainDetectorSensor::rainFrequencySnapshot(RainFrequencySnapshot& result) const {
    if (!observed_) return false;
    result = latest_;
    return true;
}
SensorOperationResult RainDetectorSensor::sample(IMeasurementSink& output) {
    Measurement measurement;
    measurement.type = MeasurementType::RainDetectorFrequency;
    measurement.valid = observed_ && latest_.window.valid()
        && static_cast<uint32_t>(clock_.nowMs()-latest_.acceptedMs) <= 1500
        && acquisition_.running();
    if (measurement.valid) measurement.value = MeasurementValue::floatingPoint(latest_.window.frequencyHz);
    measurement.quality = measurement.valid ? MeasurementQuality::Good : MeasurementQuality::Degraded;
    output.emit(measurement);
    return SensorOperationResult::Completed;
}
} // namespace EnvNode
