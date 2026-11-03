// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include "HumidityDerivedSensor.h"
#include <cmath>
namespace EnvNode {
bool humidityDerivedValues(float celsius, float rh,
    float& dewPoint, bool& hasDewPoint, float& absoluteHumidity) {
    hasDewPoint = false;
    // Restrict the approximation to its documented weather range.
    if (!std::isfinite(celsius) || celsius < -45 || celsius > 60
        || !std::isfinite(rh) || rh < 0 || rh > 100) return false;
    // Sensirion: https://sensirion.com/resource/application_note/sht/glance
    const double exponent = 17.62 * celsius / (243.12 + celsius);
    const double vapourHpa = rh / 100.0 * 6.112 * std::exp(exponent);
    absoluteHumidity = static_cast<float>(216.7 * vapourHpa / (273.15 + celsius));
    if (rh > 0) {
        const double gamma = std::log(rh / 100.0) + exponent;
        dewPoint = static_cast<float>(243.12 * gamma / (17.62 - gamma));
        hasDewPoint = std::isfinite(dewPoint);
    }
    return std::isfinite(absoluteHumidity);
}
bool HumidityDerivedSensor::supports(MeasurementType type) const {
    return sensor_->supports(type) || ((type == MeasurementType::DewPoint
        || type == MeasurementType::AbsoluteHumidity)
        && sensor_->supports(MeasurementType::Temperature)
        && sensor_->supports(MeasurementType::RelativeHumidity));
}
SensorOperationResult HumidityDerivedSensor::service(IMeasurementSink& output) { return run(output, false); }
SensorOperationResult HumidityDerivedSensor::sample(IMeasurementSink& output) { return run(output, true); }
SensorOperationResult HumidityDerivedSensor::run(IMeasurementSink& output, bool sample) {
    hasTemperature_ = hasHumidity_ = false;
    output_ = &output;
    const auto result = sample ? sensor_->sample(*this) : sensor_->service(*this);
    if (supports(MeasurementType::DewPoint) && (hasTemperature_ || hasHumidity_)) emitDerived();
    output_ = nullptr;
    return result;
}
void HumidityDerivedSensor::emit(const Measurement& input) {
    output_->emit(input);
    if (input.type == MeasurementType::Temperature) { temperature_ = input; hasTemperature_ = true; }
    if (input.type == MeasurementType::RelativeHumidity) { humidity_ = input; hasHumidity_ = true; }
}
void HumidityDerivedSensor::emitDerived() {
    float t = 0, rh = 0, dew = 0, absolute = 0;
    bool hasDew = false;
    const bool valid = hasTemperature_ && hasHumidity_
        && isMeasurementContentStructurallyValid(temperature_)
        && isMeasurementContentStructurallyValid(humidity_)
        && temperature_.valid && humidity_.valid
        && temperature_.value.tryGetFloatingPoint(t) && humidity_.value.tryGetFloatingPoint(rh)
        && humidityDerivedValues(t, rh, dew, hasDew, absolute);
    // Derivation is independent of quality: preserve the worse input quality.
    const auto quality = temperature_.quality == MeasurementQuality::Degraded
        || humidity_.quality == MeasurementQuality::Degraded ? MeasurementQuality::Degraded
        : temperature_.quality == MeasurementQuality::BelowMeasurementRange
        || humidity_.quality == MeasurementQuality::BelowMeasurementRange ? MeasurementQuality::BelowMeasurementRange
        : temperature_.quality == MeasurementQuality::Estimated
        || humidity_.quality == MeasurementQuality::Estimated ? MeasurementQuality::Estimated
        : MeasurementQuality::Good;
    for (MeasurementType type : {MeasurementType::DewPoint, MeasurementType::AbsoluteHumidity}) {
        Measurement measurement;
        measurement.type = type;
        measurement.valid = valid && (type != MeasurementType::DewPoint || hasDew);
        measurement.quality = measurement.valid ? quality : MeasurementQuality::Degraded;
        if (measurement.valid) measurement.value = MeasurementValue::floatingPoint(
            type == MeasurementType::DewPoint ? dew : absolute);
        output_->emit(measurement);
    }
}
}
