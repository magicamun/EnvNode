#pragma once

#include "ActuatorLevel.h"
#include "IOnOffActuator.h"

namespace EnvNode {

class ILevelActuator : public IOnOffActuator {
public:
    virtual ActuatorOperationResult setLevel(ActuatorLevel level) = 0;
    virtual ActuatorLevel level() const = 0;
    uint8_t outputPercent() const override { return level().percent(); }
};

} // namespace EnvNode
