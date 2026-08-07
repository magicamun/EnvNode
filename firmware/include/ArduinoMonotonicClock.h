#pragma once

#include "IMonotonicClock.h"

namespace WeatherStation {

class ArduinoMonotonicClock : public IMonotonicClock {
public:
    uint32_t nowMs() const override;
};

} // namespace WeatherStation
