#pragma once

#include <Arduino.h>
#include <cstdint>

#include "ActuatorId.h"
#include "ModuleActuatorReference.h"
#include "ControllerId.h"
#include "ControllerImplementationRegistry.h"
#include "MeasurementSourceReference.h"
#include "EnumValue.h"

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
    bool decisionOnly = false;
    MeasurementSourceReference source;
    ActuatorId targetActuatorId = InvalidActuatorId;
    // Future runtime semantics start with an Unknown decision. Values inside the
    // hysteresis band do not imply Off until an On or Off decision has existed.
    float onThreshold = 70.0F;
    float offThreshold = 65.0F;
    ThresholdDirection direction = ThresholdDirection::OnAbove;
    uint32_t maxMeasurementAgeMs = 15000;
};

struct SelectorControllerConfiguration {
    ValueId modeValueId = 0;
    ControllerId automaticControllerId = InvalidControllerId;
    ActuatorId targetActuatorId = InvalidActuatorId;
    String automaticCode;
    String onCode;
    String offCode;
};

inline bool validSelectorMapping(const SelectorControllerConfiguration& selector,
    const EnumValueConfiguration& value) {
    if (selector.modeValueId != value.id || !validEnumValueConfiguration(value)
        || selector.automaticCode == selector.onCode || selector.automaticCode == selector.offCode
        || selector.onCode == selector.offCode) return false;
    bool automatic = false, on = false, off = false;
    for (const auto& option : value.options) {
        automatic |= option.code == selector.automaticCode;
        on |= option.code == selector.onCode;
        off |= option.code == selector.offCode;
    }
    return automatic && on && off;
}

inline uint32_t selectorMappingSignature(const SelectorControllerConfiguration& selector) {
    uint32_t hash = 2166136261u;
    const String* codes[] = {&selector.automaticCode, &selector.onCode, &selector.offCode};
    for (const auto* code : codes) {
        for (size_t i = 0; i < code->length(); ++i) hash = (hash ^ static_cast<uint8_t>((*code)[i])) * 16777619u;
        hash = (hash ^ 0xffu) * 16777619u;
    }
    return hash;
}

struct ControllerImplementationConfiguration {
    SelectorControllerConfiguration selector;
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
        case ControllerImplementation::Selector:
            targetActuatorId = slot.implementationConfiguration.selector.targetActuatorId;
            return true;
        case ControllerImplementation::Threshold:
            if (slot.implementationConfiguration.threshold.decisionOnly) {
                targetActuatorId = InvalidActuatorId;
                return false;
            }
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
    if (slot.implementation == ControllerImplementation::Threshold
        && slot.implementationConfiguration.threshold.decisionOnly) return nullptr;
    return validModuleActuatorReference(slot.moduleTarget)
        ? &slot.moduleTarget : nullptr;
}

constexpr size_t MaxControllerSlotCount = 16;
constexpr size_t MaxControllerSlotNameLength = 32;

} // namespace EnvNode
