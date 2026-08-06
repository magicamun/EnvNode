#pragma once

#include <Arduino.h>

namespace WeatherStation {

class IMqttService {
public:
    virtual ~IMqttService() = default;

    virtual void begin() = 0;
    virtual void loop() = 0;
    virtual bool connected() const = 0;
};

} // namespace WeatherStation
