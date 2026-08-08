#pragma once

#include <cstdint>

#include <DHT.h>

#include "IMonotonicClock.h"
#include "ISensor.h"
#include "Logger.h"

namespace WeatherStation {

class AM2302Sensor : public ISensor {
public:
    AM2302Sensor(
        SensorId id,
        uint8_t dataPin,
        IMonotonicClock& monotonicClock,
        ILogger& logger);

    SensorId id() const override;
    const char* type() const override;
    SensorProvenance provenance() const override;
    SensorState state() const override;
    bool supports(MeasurementType type) const override;

    void begin() override;
    SensorOperationResult service(IMeasurementSink& output) override;
    SensorOperationResult sample(IMeasurementSink& output) override;

private:
    static bool isTemperatureValid(float temperatureCelsius);
    static bool isHumidityValid(float relativeHumidityPercent);
    void emitMeasurement(IMeasurementSink& output, MeasurementType type, float value);

    SensorId id_;
    uint8_t dataPin_;
    IMonotonicClock& monotonicClock_;
    ILogger& logger_;
    DHT dht_;
    SensorState state_ = SensorState::Unknown;
    uint32_t readyAtMs_ = 0;
};

} // namespace WeatherStation
