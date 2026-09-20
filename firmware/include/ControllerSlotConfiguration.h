#pragma once

#include <Arduino.h>
#include <cstdint>

#include "ActuatorId.h"
#include "ControllerId.h"
#include "ControllerImplementationRegistry.h"
#include "MeasurementSourceReference.h"

namespace EnvNode {

struct ModuleActuatorReference {
    uint8_t moduleInstanceFingerprint[8] = {};
    uint8_t deviceIdHash[4] = {};
};

inline void setModuleActuatorInstanceId(
    ModuleActuatorReference& reference, const uint8_t* instanceId) {
    memcpy(reference.moduleInstanceFingerprint, instanceId,
        sizeof(reference.moduleInstanceFingerprint));
}

inline void setModuleActuatorDeviceId(
    ModuleActuatorReference& reference, const char* deviceId) {
    uint32_t hash = 2166136261UL;
    if (deviceId != nullptr) {
        while (*deviceId != '\0') {
            hash ^= static_cast<uint8_t>(*deviceId++);
            hash *= 16777619UL;
        }
    }
    for (size_t index = 0; index < sizeof(reference.deviceIdHash); ++index) {
        reference.deviceIdHash[index] = static_cast<uint8_t>(hash >> (index * 8));
    }
}

inline bool validModuleActuatorReference(const ModuleActuatorReference& reference) {
    bool nonzero = false;
    for (size_t index = 0; index < sizeof(reference.moduleInstanceFingerprint); ++index) {
        nonzero = nonzero || reference.moduleInstanceFingerprint[index] != 0;
    }
    bool nonzeroDevice = false;
    for (size_t index = 0; index < sizeof(reference.deviceIdHash); ++index) {
        nonzeroDevice = nonzeroDevice || reference.deviceIdHash[index] != 0;
    }
    return nonzero && nonzeroDevice;
}

inline bool sameModuleActuatorReference(
    const ModuleActuatorReference& left,
    const ModuleActuatorReference& right) {
    if (!validModuleActuatorReference(left) || !validModuleActuatorReference(right)) {
        return false;
    }
    for (size_t index = 0; index < sizeof(left.moduleInstanceFingerprint); ++index) {
        if (left.moduleInstanceFingerprint[index]
            != right.moduleInstanceFingerprint[index]) return false;
    }
    return memcmp(left.deviceIdHash, right.deviceIdHash,
        sizeof(left.deviceIdHash)) == 0;
}

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
