#include "SHT4xSensor.h"

#include <cmath>

namespace EnvNode {
namespace {
constexpr float MinimumTemperatureCelsius = -40.0F;
constexpr float MaximumTemperatureCelsius = 125.0F;
constexpr float MinimumRelativeHumidityPercent = 0.0F;
constexpr float MaximumRelativeHumidityPercent = 100.0F;
}

SHT4xSensor::SHT4xSensor(SensorId id, I2CResource resource,
    I2CBusManager& i2cBusManager, ILogger& logger)
    : id_(id), resource_(resource), i2cBusManager_(i2cBusManager), logger_(logger) {
}

SensorId SHT4xSensor::id() const { return id_; }
const char* SHT4xSensor::type() const { return "SHT4x"; }
SensorProvenance SHT4xSensor::provenance() const { return SensorProvenance::Physical; }
SensorState SHT4xSensor::state() const { return state_; }

bool SHT4xSensor::supports(MeasurementType type) const {
    return type == MeasurementType::Temperature
        || type == MeasurementType::RelativeHumidity;
}

void SHT4xSensor::begin() {
    state_ = SensorState::Initializing;
    logger_.printf("SHT4x sensor %u initializing on %s address 0x%02X\n",
        id_, i2cBusName(resource_.bus), resource_.address);
    TwoWire* wire = i2cBusManager_.wire(resource_.bus);
    if (wire == nullptr || !sht4x_.begin(wire)) {
        state_ = SensorState::Failed;
        logger_.printf("SHT4x sensor %u initialization failed on %s address 0x%02X\n",
            id_, i2cBusName(resource_.bus), resource_.address);
        return;
    }
    state_ = SensorState::Ready;
    logger_.printf("SHT4x sensor %u ready\n", id_);
}

SensorOperationResult SHT4xSensor::service(IMeasurementSink& output) {
    (void)output;
    return SensorOperationResult::NoData;
}

SensorOperationResult SHT4xSensor::sample(IMeasurementSink& output) {
    if (state_ != SensorState::Ready && state_ != SensorState::Degraded) {
        return SensorOperationResult::NoData;
    }
    sensors_event_t humidityEvent = {};
    sensors_event_t temperatureEvent = {};
    const bool readSucceeded = sht4x_.getEvent(&humidityEvent, &temperatureEvent);
    const float temperature = temperatureEvent.temperature;
    const float humidity = humidityEvent.relative_humidity;
    const bool temperatureValid = readSucceeded && validTemperature(temperature);
    const bool humidityValid = readSucceeded && validHumidity(humidity);
    emit(output, MeasurementType::Temperature, temperature, temperatureValid);
    emit(output, MeasurementType::RelativeHumidity, humidity, humidityValid);
    if (!temperatureValid || !humidityValid) {
        if (state_ != SensorState::Degraded) logger_.printf("SHT4x sensor %u read failure\n", id_);
        state_ = SensorState::Degraded;
        return SensorOperationResult::HardwareFailure;
    }
    if (state_ == SensorState::Degraded) {
        logger_.printf("SHT4x sensor %u recovered after read failure\n", id_);
    }
    state_ = SensorState::Ready;
    return SensorOperationResult::Completed;
}

bool SHT4xSensor::validTemperature(float value) {
    return std::isfinite(value) && value >= MinimumTemperatureCelsius
        && value <= MaximumTemperatureCelsius;
}
bool SHT4xSensor::validHumidity(float value) {
    return std::isfinite(value) && value >= MinimumRelativeHumidityPercent
        && value <= MaximumRelativeHumidityPercent;
}
void SHT4xSensor::emit(IMeasurementSink& output, MeasurementType type, float value, bool valid) {
    Measurement measurement;
    measurement.type = type;
    measurement.valid = valid;
    measurement.quality = valid ? MeasurementQuality::Good : MeasurementQuality::Degraded;
    if (valid) measurement.value = MeasurementValue::floatingPoint(value);
    output.emit(measurement);
}

} // namespace EnvNode
