#include "ArduinoMonotonicClock.h"

#include <Arduino.h>

namespace WeatherStation {

uint32_t ArduinoMonotonicClock::nowMs() const {
    return millis();
}

} // namespace WeatherStation
