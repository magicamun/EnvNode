#include "ArduinoMonotonicClock.h"

#include <Arduino.h>

namespace EnvNode {

uint32_t ArduinoMonotonicClock::nowMs() const {
    return millis();
}

} // namespace EnvNode
