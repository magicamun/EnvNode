#pragma once
#include "ActuatorSlotConfiguration.h"
#include "IOnOffActuatorResolver.h"
namespace EnvNode {
inline bool controllerTargetsSameActuator(const ControllerSlotConfiguration& a,
    const ControllerSlotConfiguration& b, const ActuatorSlotConfiguration* actuators = nullptr,
    const IOnOffActuatorResolver* resolver = nullptr) {
    ActuatorId ids[2];
    if (!configuredControllerTargetActuatorId(a, ids[0])
        || !configuredControllerTargetActuatorId(b, ids[1])) return false;
    const ControllerSlotConfiguration* slots[] = {&a, &b};
    ModuleActuatorReference refs[2];
    bool module[2] = {};
    for (size_t i = 0; i < 2; ++i) {
        const auto* saved = configuredControllerModuleTarget(*slots[i]);
        if (saved) { refs[i] = *saved; module[i] = true; continue; }
        if (actuators && ids[i] > 0 && ids[i] <= MaxActuatorSlotCount) {
            refs[i] = actuators[ids[i] - 1].moduleTarget;
            module[i] = validModuleActuatorReference(refs[i]);
        }
        if (!module[i] && resolver) module[i] = resolver->moduleReference(ids[i], refs[i]);
    }
    if (module[0] && module[1]) return sameModuleActuatorReference(refs[0], refs[1]);
    return !module[0] && !module[1] && isValidActuatorId(ids[0]) && ids[0] == ids[1];
}
}
