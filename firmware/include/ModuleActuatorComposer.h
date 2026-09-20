#pragma once

#include "ActuatorRuntime.h"
#include "ModuleDeviceInventory.h"
#include "ModuleDiscoveryService.h"
#include "SensorSlotConfiguration.h"

namespace EnvNode {

class ModuleActuatorComposer {
public:
    size_t compose(
        const ModuleDiscoveryService& discovery,
        const BoardProfile& board,
        const SensorSlotConfiguration* sensors);

    const AutomaticActuatorDefinition* definitions() const;
    size_t count() const;

private:
    AutomaticActuatorDefinition definitions_[MaxActuatorSlotCount];
    size_t count_ = 0;
};

} // namespace EnvNode
