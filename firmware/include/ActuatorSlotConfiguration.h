#pragma once

#include <Arduino.h>

#include "ActuatorId.h"
#include "ActuatorImplementationRegistry.h"
#include "HardwareResources.h"

namespace EnvNode {

struct ActuatorSlotConfiguration {
    ActuatorId slotId = InvalidActuatorId;
    bool enabled = false;
    String name;
    ActuatorImplementation implementation = ActuatorImplementation::None;
    HardwareResourceAssignment hardware;
};

constexpr size_t MaxActuatorSlotCount = 16;
constexpr size_t MaxActuatorSlotNameLength = 32;

} // namespace EnvNode
