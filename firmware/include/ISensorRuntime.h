#pragma once

#include <cstddef>

namespace WeatherStation {

class ISensorRuntime {
public:
    virtual ~ISensorRuntime() = default;
    virtual bool rebuild(size_t& activeSensorCount, const char*& failureReason) = 0;
};

} // namespace WeatherStation
