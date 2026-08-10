#pragma once

#include <Adafruit_BME280.h>

#include "ISensor.h"
#include "Logger.h"
#include "HardwareResources.h"
#include "I2CBusManager.h"

namespace EnvNode {

class BME280Sensor : public ISensor {
public:
    BME280Sensor(SensorId id, I2CResource resource, I2CBusManager& i2cBusManager, ILogger& logger);

    SensorId id() const override;
    const char* type() const override;
    SensorProvenance provenance() const override;
    SensorState state() const override;
    bool supports(MeasurementType type) const override;

    void begin() override;
    SensorOperationResult service(IMeasurementSink& output) override;
    SensorOperationResult sample(IMeasurementSink& output) override;

private:
    static bool validTemperature(float value);
    static bool validHumidity(float value);
    static bool validPressure(float value);
    static void emit(IMeasurementSink& output, MeasurementType type, float value, bool valid);

    SensorId id_;
    I2CResource resource_;
    I2CBusManager& i2cBusManager_;
    ILogger& logger_;
    Adafruit_BME280 bme280_;
    SensorState state_ = SensorState::Unknown;
};

} // namespace EnvNode
