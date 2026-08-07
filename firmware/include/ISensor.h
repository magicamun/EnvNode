#pragma once

#include "Measurement.h"
#include "SensorState.h"

namespace WeatherStation {

class ISensor {
public:
    virtual ~ISensor() = default;

    virtual void begin() = 0;
    virtual void loop() = 0;
    virtual SensorId id() const = 0;
    virtual SensorState state() const = 0;
    virtual bool available() const = 0;
    virtual bool read(Measurement& outMeasurement) = 0;
};

} // namespace WeatherStation
