#pragma once

#include <Arduino.h>
#include <cstdint>

#include "ActuatorId.h"
#include "ControllerId.h"
#include "ControllerImplementationRegistry.h"
#include "MeasurementSourceReference.h"

namespace EnvNode {

struct BlinkControllerConfiguration {
    ActuatorId targetActuatorId = InvalidActuatorId;
    uint32_t onDurationMs = 1000;
    uint32_t offDurationMs = 1000;
};

struct ThresholdControllerConfiguration {
    MeasurementSourceReference source;
    ActuatorId targetActuatorId = InvalidActuatorId;
    // Future runtime semantics start with an Unknown decision. Values inside the
    // hysteresis band do not imply Off until an On or Off decision has existed.
    float onThreshold = 70.0F;
    float offThreshold = 65.0F;
    uint32_t maxMeasurementAgeMs = 15000;
};

struct ControllerImplementationConfiguration {
    BlinkControllerConfiguration blink;
    ThresholdControllerConfiguration threshold;
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
