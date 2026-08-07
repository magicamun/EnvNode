#pragma once

#include <Arduino.h>

namespace WeatherStation {

class ITimeService {
public:
    virtual ~ITimeService() = default;

    virtual void begin() = 0;
    virtual void loop() = 0;
    virtual bool synchronized() const = 0;
    virtual time_t now() const = 0;
    virtual bool localCivilTime(tm& localTime) const = 0;
    virtual String iso8601Utc() const = 0;
    virtual String iso8601Local() const = 0;
    virtual String iso8601Local(time_t timestamp) const = 0;
    virtual uint32_t epoch() const = 0;
};

} // namespace WeatherStation
