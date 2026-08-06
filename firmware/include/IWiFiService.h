#pragma once

#include <Arduino.h>

namespace WeatherStation {

class IWiFiService {
public:
    virtual ~IWiFiService() = default;

    virtual void begin() = 0;
    virtual void loop() = 0;
    virtual bool connected() const = 0;
    virtual bool inSetupAccessPointMode() const = 0;
    virtual String ipAddress() const = 0;
};

} // namespace WeatherStation
