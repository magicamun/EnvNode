#pragma once

#include "ActuatorId.h"
#include "IOnOffActuator.h"
#include "ControllerSlotConfiguration.h"

namespace EnvNode {

class IOnOffActuatorResolver {
public:
    virtual ~IOnOffActuatorResolver() = default;
    virtual IOnOffActuator* onOffActuator(ActuatorId id) = 0;
    virtual bool moduleReference(ActuatorId, ModuleActuatorReference&) const { return false; }
    virtual IOnOffActuator* onOffActuator(
        const ModuleActuatorReference& reference) {
        (void)reference;
        return nullptr;
    }
};

} // namespace EnvNode
