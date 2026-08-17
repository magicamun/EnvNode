#pragma once

#include "ActuatorId.h"
#include "IOnOffActuator.h"

namespace EnvNode {

class IOnOffActuatorResolver {
public:
    virtual ~IOnOffActuatorResolver() = default;
    virtual IOnOffActuator* onOffActuator(ActuatorId id) = 0;
};

} // namespace EnvNode
