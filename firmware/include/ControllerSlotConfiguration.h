#pragma once

#include <Arduino.h>
#include <cstdint>

#include "ActuatorId.h"
#include "ModuleActuatorReference.h"
#include "ControllerId.h"
#include "ControllerImplementationRegistry.h"
#include "MeasurementSourceReference.h"

namespace EnvNode {

struct BlinkControllerConfiguration {
    ActuatorId targetActuatorId = InvalidActuatorId;
    uint32_t onDurationMs = 1000;
    uint32_t offDurationMs = 1000;
};

enum class ThresholdDirection : uint8_t { OnAbove = 0, OnBelow = 1 };

inline const char* thresholdDirectionStableName(ThresholdDirection value) {
    return value == ThresholdDirection::OnBelow ? "on_below" : "on_above";
}

inline const char* thresholdOnComparisonSymbol(ThresholdDirection direction) {
    return direction == ThresholdDirection::OnBelow ? "≤" : "≥";
}

inline const char* thresholdOffComparisonSymbol(ThresholdDirection direction) {
    return direction == ThresholdDirection::OnBelow ? "≥" : "≤";
}

inline bool validThresholdOrdering(
    ThresholdDirection direction, float onThreshold, float offThreshold) {
    return direction == ThresholdDirection::OnAbove
        ? onThreshold > offThreshold
        : direction == ThresholdDirection::OnBelow && onThreshold < offThreshold;
}

struct ThresholdControllerConfiguration {
    MeasurementSourceReference source;
    ActuatorId targetActuatorId = InvalidActuatorId;
    // Future runtime semantics start with an Unknown decision. Values inside the
    // hysteresis band do not imply Off until an On or Off decision has existed.
    float onThreshold = 70.0F;
    float offThreshold = 65.0F;
    ThresholdDirection direction = ThresholdDirection::OnAbove;
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
    ModuleActuatorReference moduleTarget;
    ControllerImplementationConfiguration implementationConfiguration;
};

inline bool configuredControllerTargetActuatorId(
    const ControllerSlotConfiguration& slot,
    ActuatorId& targetActuatorId) {
    switch (slot.implementation) {
        case ControllerImplementation::Blink:
            targetActuatorId =
                slot.implementationConfiguration.blink.targetActuatorId;
            return true;
        case ControllerImplementation::Threshold:
            targetActuatorId =
                slot.implementationConfiguration.threshold.targetActuatorId;
            return true;
        case ControllerImplementation::None:
        default:
            targetActuatorId = InvalidActuatorId;
            return false;
    }
}

inline const ModuleActuatorReference* configuredControllerModuleTarget(
    const ControllerSlotConfiguration& slot) {
    return validModuleActuatorReference(slot.moduleTarget)
        ? &slot.moduleTarget : nullptr;
}

constexpr size_t MaxControllerSlotCount = 16;
constexpr size_t MaxControllerSlotNameLength = 32;

} // namespace EnvNode
