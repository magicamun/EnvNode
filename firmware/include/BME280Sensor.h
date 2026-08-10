#pragma once

#include <Adafruit_BME280.h>

#include "ISensor.h"
#include "Logger.h"

namespace WeatherStation {

class BME280Sensor : public ISensor {
public:
    BME280Sensor(SensorId id, uint8_t i2cAddress, ILogger& logger);

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
    uint8_t i2cAddress_;
    ILogger& logger_;
    Adafruit_BME280 bme280_;
    SensorState state_ = SensorState::Unknown;
};

} // namespace WeatherStation
