#pragma once

#include <cstdint>

namespace WeatherStation {

class IMonotonicClock {
public:
    virtual ~IMonotonicClock() = default;

    virtual uint32_t nowMs() const = 0;
};

} // namespace WeatherStation
