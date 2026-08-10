#include "AM2302Sensor.h"

#include <cmath>

namespace EnvNode {
namespace {

const float MinimumTemperatureCelsius = -40.0F;
const float MaximumTemperatureCelsius = 80.0F;
const float MinimumRelativeHumidityPercent = 0.0F;
const float MaximumRelativeHumidityPercent = 100.0F;
const uint32_t StartupStabilizationMs = 1000;

} // namespace

AM2302Sensor::AM2302Sensor(
    SensorId id,
    uint8_t dataPin,
    IMonotonicClock& monotonicClock,
    ILogger& logger)
    : id_(id)
    , dataPin_(dataPin)
    , monotonicClock_(monotonicClock)
    , logger_(logger)
    , dht_(dataPin, DHT22) {
}

SensorId AM2302Sensor::id() const {
    return id_;
}

const char* AM2302Sensor::type() const {
    return "AM2302 / DHT22";
}

SensorProvenance AM2302Sensor::provenance() const {
    return SensorProvenance::Physical;
}

SensorState AM2302Sensor::state() const {
    return state_;
}

bool AM2302Sensor::supports(MeasurementType type) const {
    return type == MeasurementType::Temperature
        || type == MeasurementType::RelativeHumidity;
}

void AM2302Sensor::begin() {
    state_ = SensorState::Initializing;
    logger_.printf("AM2302 sensor %u initializing on GPIO%u\n", id_, dataPin_);
    dht_.begin();
    readyAtMs_ = monotonicClock_.nowMs() + StartupStabilizationMs;
}

SensorOperationResult AM2302Sensor::service(IMeasurementSink& output) {
    (void)output;
    if (state_ == SensorState::Initializing
        && static_cast<int32_t>(monotonicClock_.nowMs() - readyAtMs_) >= 0) {
        state_ = SensorState::Ready;
        logger_.printf("AM2302 sensor %u ready\n", id_);
    }
    return SensorOperationResult::NoData;
}

SensorOperationResult AM2302Sensor::sample(IMeasurementSink& output) {
    if (state_ != SensorState::Ready && state_ != SensorState::Degraded) {
        return SensorOperationResult::NoData;
    }

    // Adafruit DHT caches reads inside the device's minimum interval. These two
    // calls therefore use one acquisition rather than initiating two bus reads.
    const float humidity = dht_.readHumidity();
    const float temperature = dht_.readTemperature(false);
    const bool humidityValid = isHumidityValid(humidity);
    const bool temperatureValid = isTemperatureValid(temperature);

    if (temperatureValid) {
        emitMeasurement(output, MeasurementType::Temperature, temperature);
    }
    if (humidityValid) {
        emitMeasurement(output, MeasurementType::RelativeHumidity, humidity);
    }

    if (!temperatureValid || !humidityValid) {
        if (state_ != SensorState::Degraded) {
            logger_.printf(
                "AM2302 sensor %u read failure: temperatureValid=%s humidityValid=%s\n",
                id_, temperatureValid ? "true" : "false", humidityValid ? "true" : "false");
        }
        state_ = SensorState::Degraded;
        return SensorOperationResult::HardwareFailure;
    }

    if (state_ == SensorState::Degraded) {
        logger_.printf("AM2302 sensor %u recovered after read failure\n", id_);
    }
    state_ = SensorState::Ready;
    return SensorOperationResult::Completed;
}

bool AM2302Sensor::isTemperatureValid(float temperatureCelsius) {
    return std::isfinite(temperatureCelsius)
        && temperatureCelsius >= MinimumTemperatureCelsius
        && temperatureCelsius <= MaximumTemperatureCelsius;
}

bool AM2302Sensor::isHumidityValid(float relativeHumidityPercent) {
    return std::isfinite(relativeHumidityPercent)
        && relativeHumidityPercent >= MinimumRelativeHumidityPercent
        && relativeHumidityPercent <= MaximumRelativeHumidityPercent;
}

void AM2302Sensor::emitMeasurement(
    IMeasurementSink& output,
    MeasurementType type,
    float value) {
    Measurement measurement;
    measurement.type = type;
    measurement.value = MeasurementValue::floatingPoint(value);
    measurement.valid = true;
    measurement.quality = MeasurementQuality::Good;
    output.emit(measurement);
}

} // namespace EnvNode
