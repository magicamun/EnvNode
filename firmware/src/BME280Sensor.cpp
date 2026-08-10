#include "BME280Sensor.h"

#include <cmath>

namespace WeatherStation {
namespace {

constexpr float MinimumTemperatureCelsius = -40.0F;
constexpr float MaximumTemperatureCelsius = 85.0F;
constexpr float MinimumRelativeHumidityPercent = 0.0F;
constexpr float MaximumRelativeHumidityPercent = 100.0F;
constexpr float MinimumPressurePascal = 30000.0F;
constexpr float MaximumPressurePascal = 110000.0F;

} // namespace

BME280Sensor::BME280Sensor(SensorId id, uint8_t i2cAddress, ILogger& logger)
    : id_(id)
    , i2cAddress_(i2cAddress)
    , logger_(logger) {
}

SensorId BME280Sensor::id() const { return id_; }
const char* BME280Sensor::type() const { return "BME280"; }
SensorProvenance BME280Sensor::provenance() const { return SensorProvenance::Physical; }
SensorState BME280Sensor::state() const { return state_; }

bool BME280Sensor::supports(MeasurementType type) const {
    return type == MeasurementType::Temperature
        || type == MeasurementType::RelativeHumidity
        || type == MeasurementType::AtmosphericPressure;
}

void BME280Sensor::begin() {
    state_ = SensorState::Initializing;
    logger_.printf("BME280 sensor %u initializing on I2C address 0x%02X\n", id_, i2cAddress_);
    if (!bme280_.begin(i2cAddress_)) {
        state_ = SensorState::Failed;
        logger_.printf("BME280 sensor %u initialization failed on I2C address 0x%02X\n", id_, i2cAddress_);
        return;
    }
    state_ = SensorState::Ready;
    logger_.printf("BME280 sensor %u ready\n", id_);
}

SensorOperationResult BME280Sensor::service(IMeasurementSink& output) {
    (void)output;
    return SensorOperationResult::NoData;
}

SensorOperationResult BME280Sensor::sample(IMeasurementSink& output) {
    if (state_ != SensorState::Ready && state_ != SensorState::Degraded) {
        return SensorOperationResult::NoData;
    }

    const float temperature = bme280_.readTemperature();
    const float humidity = bme280_.readHumidity();
    const float pressure = bme280_.readPressure();
    const bool temperatureValid = validTemperature(temperature);
    const bool humidityValid = validHumidity(humidity);
    const bool pressureValid = validPressure(pressure);

    emit(output, MeasurementType::Temperature, temperature, temperatureValid);
    emit(output, MeasurementType::RelativeHumidity, humidity, humidityValid);
    emit(output, MeasurementType::AtmosphericPressure, pressure, pressureValid);

    if (!temperatureValid || !humidityValid || !pressureValid) {
        if (state_ != SensorState::Degraded) {
            logger_.printf("BME280 sensor %u read failure\n", id_);
        }
        state_ = SensorState::Degraded;
        return SensorOperationResult::HardwareFailure;
    }
    if (state_ == SensorState::Degraded) {
        logger_.printf("BME280 sensor %u recovered after read failure\n", id_);
    }
    state_ = SensorState::Ready;
    return SensorOperationResult::Completed;
}

bool BME280Sensor::validTemperature(float value) {
    return std::isfinite(value) && value >= MinimumTemperatureCelsius
        && value <= MaximumTemperatureCelsius;
}

bool BME280Sensor::validHumidity(float value) {
    return std::isfinite(value) && value >= MinimumRelativeHumidityPercent
        && value <= MaximumRelativeHumidityPercent;
}

bool BME280Sensor::validPressure(float value) {
    return std::isfinite(value) && value >= MinimumPressurePascal
        && value <= MaximumPressurePascal;
}

void BME280Sensor::emit(
    IMeasurementSink& output, MeasurementType type, float value, bool valid) {
    Measurement measurement;
    measurement.type = type;
    measurement.valid = valid;
    measurement.quality = valid ? MeasurementQuality::Good : MeasurementQuality::Degraded;
    if (valid) measurement.value = MeasurementValue::floatingPoint(value);
    output.emit(measurement);
}

} // namespace WeatherStation
