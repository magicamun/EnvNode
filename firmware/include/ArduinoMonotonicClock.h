#pragma once

#include "IMonotonicClock.h"

namespace EnvNode {

class ArduinoMonotonicClock : public IMonotonicClock {
public:
    uint32_t nowMs() const override;
};

} // namespace EnvNode
