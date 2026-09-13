#include "SHTC3Sensor.h"

#include <cmath>

namespace EnvNode {
namespace {
constexpr float MinimumTemperatureCelsius = -40.0F;
constexpr float MaximumTemperatureCelsius = 125.0F;
constexpr float MinimumRelativeHumidityPercent = 0.0F;
constexpr float MaximumRelativeHumidityPercent = 100.0F;
}

SHTC3Sensor::SHTC3Sensor(SensorId id, I2CResource resource,
    I2CBusManager& i2cBusManager, ILogger& logger)
    : id_(id), resource_(resource), i2cBusManager_(i2cBusManager), logger_(logger) {
}

SensorId SHTC3Sensor::id() const { return id_; }
const char* SHTC3Sensor::type() const { return "SHTC3"; }
SensorProvenance SHTC3Sensor::provenance() const { return SensorProvenance::Physical; }
SensorState SHTC3Sensor::state() const { return state_; }

bool SHTC3Sensor::supports(MeasurementType type) const {
    return type == MeasurementType::Temperature
        || type == MeasurementType::RelativeHumidity;
}

void SHTC3Sensor::begin() {
    state_ = SensorState::Initializing;
    logger_.debugf("SHTC3 sensor %u initializing on %s address 0x%02X",
        id_, i2cBusName(resource_.bus), resource_.address);
    TwoWire* wire = i2cBusManager_.wire(resource_.bus);
    if (resource_.address != I2CAddress || wire == nullptr || !shtc3_.begin(wire)) {
        state_ = SensorState::Failed;
        logger_.errorf("SHTC3 sensor %u initialization failed on %s address 0x%02X",
            id_, i2cBusName(resource_.bus), resource_.address);
        return;
    }
    state_ = SensorState::Ready;
    logger_.infof("SHTC3 sensor %u ready", id_);
}

SensorOperationResult SHTC3Sensor::service(IMeasurementSink& output) {
    (void)output;
    return SensorOperationResult::NoData;
}

SensorOperationResult SHTC3Sensor::sample(IMeasurementSink& output) {
    if (state_ != SensorState::Ready && state_ != SensorState::Degraded) {
        return SensorOperationResult::NoData;
    }
    sensors_event_t humidityEvent = {};
    sensors_event_t temperatureEvent = {};
    const bool readSucceeded = shtc3_.getEvent(&humidityEvent, &temperatureEvent);
    const float temperature = temperatureEvent.temperature;
    const float humidity = humidityEvent.relative_humidity;
    const bool temperatureValid = readSucceeded && validTemperature(temperature);
    const bool humidityValid = readSucceeded && validHumidity(humidity);
    emit(output, MeasurementType::Temperature, temperature, temperatureValid);
    emit(output, MeasurementType::RelativeHumidity, humidity, humidityValid);
    if (!temperatureValid || !humidityValid) {
        if (state_ != SensorState::Degraded) logger_.warnf("SHTC3 sensor %u read failure", id_);
        state_ = SensorState::Degraded;
        return SensorOperationResult::HardwareFailure;
    }
    if (state_ == SensorState::Degraded) {
        logger_.infof("SHTC3 sensor %u recovered after read failure", id_);
    }
    state_ = SensorState::Ready;
    return SensorOperationResult::Completed;
}

bool SHTC3Sensor::validTemperature(float value) {
    return std::isfinite(value) && value >= MinimumTemperatureCelsius
        && value <= MaximumTemperatureCelsius;
}

bool SHTC3Sensor::validHumidity(float value) {
    return std::isfinite(value) && value >= MinimumRelativeHumidityPercent
        && value <= MaximumRelativeHumidityPercent;
}

void SHTC3Sensor::emit(
    IMeasurementSink& output, MeasurementType type, float value, bool valid) {
    Measurement measurement;
    measurement.type = type;
    measurement.valid = valid;
    measurement.quality = valid ? MeasurementQuality::Good : MeasurementQuality::Degraded;
    if (valid) measurement.value = MeasurementValue::floatingPoint(value);
    output.emit(measurement);
}

} // namespace EnvNode
