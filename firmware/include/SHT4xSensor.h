#pragma once

#include <Adafruit_SHT4x.h>

#include "HardwareResources.h"
#include "I2CBusManager.h"
#include "ISensor.h"
#include "Logger.h"

namespace EnvNode {

class SHT4xSensor : public ISensor {
public:
    SHT4xSensor(SensorId id, I2CResource resource,
        I2CBusManager& i2cBusManager, ILogger& logger);

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
    static void emit(IMeasurementSink& output, MeasurementType type, float value, bool valid);

    SensorId id_;
    I2CResource resource_;
    I2CBusManager& i2cBusManager_;
    ILogger& logger_;
    Adafruit_SHT4x sht4x_;
    SensorState state_ = SensorState::Unknown;
};

} // namespace EnvNode
