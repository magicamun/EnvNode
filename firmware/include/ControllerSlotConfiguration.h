#pragma once

#include <Arduino.h>
#include <cstdint>

#include "ActuatorId.h"
#include "ControllerId.h"
#include "ControllerImplementationRegistry.h"

namespace EnvNode {

struct BlinkControllerConfiguration {
    ActuatorId targetActuatorId = InvalidActuatorId;
    uint32_t onDurationMs = 1000;
    uint32_t offDurationMs = 1000;
};

struct ControllerImplementationConfiguration {
    BlinkControllerConfiguration blink;
};

struct ControllerSlotConfiguration {
    ControllerId slotId = InvalidControllerId;
    bool enabled = false;
    String name;
    ControllerImplementation implementation = ControllerImplementation::None;
    ControllerImplementationConfiguration implementationConfiguration;
};

constexpr size_t MaxControllerSlotCount = 16;
constexpr size_t MaxControllerSlotNameLength = 32;

} // namespace EnvNode
