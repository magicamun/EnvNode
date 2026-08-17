#pragma once

#include <Arduino.h>

#include "ActuatorSlotConfiguration.h"
#include "ControllerSlotConfiguration.h"

namespace EnvNode {

bool buildActuatorDescription(
    const ActuatorSlotConfiguration& slot,
    String& output);
bool buildControllerDescription(
    const ControllerSlotConfiguration& slot,
    String& output);

} // namespace EnvNode
