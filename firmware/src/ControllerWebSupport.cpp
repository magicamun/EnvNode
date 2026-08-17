#include "ControllerWebSupport.h"

#include <climits>
#include <cmath>
#include <cstdlib>

#include "ActuatorImplementationRegistry.h"
#include "SensorImplementationRegistry.h"

namespace EnvNode {
namespace {

bool parseUnsignedId(const String& text, uint32_t maximum, uint32_t& result) {
    if (text.isEmpty()) return false;
    char* end = nullptr;
    const unsigned long parsed = strtoul(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0' || parsed == 0 || parsed > maximum) {
        return false;
    }
    result = static_cast<uint32_t>(parsed);
    return true;
}

bool parseFiniteFloat(const String& text, float& result) {
    if (text.isEmpty()) return false;
    char* end = nullptr;
    const float parsed = strtof(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0' || !std::isfinite(parsed)) return false;
    result = parsed;
    return true;
}

} // namespace

bool isThresholdCompatibleMeasurementType(MeasurementType type) {
    const MeasurementTypeMetadata& metadata = measurementTypeMetadata(type);
    return metadata.expectedValueKind == ValueKind::FloatingPoint
        && metadata.semantics == MeasurementSemantics::State;
}

bool sensorSupportsThresholdMeasurement(
    const SensorSlotConfiguration& slot,
    MeasurementType type) {
    if (!isEligibleThresholdSensor(slot)
        || !isThresholdCompatibleMeasurementType(type)) {
        return false;
    }
    const SensorImplementationMetadata* metadata =
        SensorImplementationRegistry::find(slot.implementation);
    for (size_t index = 0; index < metadata->measurementTypeCount; ++index) {
        if (metadata->measurementTypes[index] == type) return true;
    }
    return false;
}

size_t thresholdMeasurementTypesForSensor(
    const SensorSlotConfiguration& slot,
    MeasurementType* types,
    size_t capacity) {
    if (!isEligibleThresholdSensor(slot) || types == nullptr || capacity == 0) {
        return 0;
    }
    const SensorImplementationMetadata* metadata =
        SensorImplementationRegistry::find(slot.implementation);
    size_t count = 0;
    for (size_t index = 0; index < metadata->measurementTypeCount; ++index) {
        const MeasurementType type = metadata->measurementTypes[index];
        if (!isThresholdCompatibleMeasurementType(type)) continue;
        bool duplicate = false;
        for (size_t existing = 0; existing < count; ++existing) {
            if (types[existing] == type) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate && count < capacity) types[count++] = type;
    }
    return count;
}

bool isEligibleThresholdSensor(const SensorSlotConfiguration& slot) {
    if (!slot.enabled || slot.implementation == SensorImplementation::None) return false;
    const SensorImplementationMetadata* metadata =
        SensorImplementationRegistry::find(slot.implementation);
    if (metadata == nullptr) return false;
    for (size_t index = 0; index < metadata->measurementTypeCount; ++index) {
        if (isThresholdCompatibleMeasurementType(metadata->measurementTypes[index])) {
            return true;
        }
    }
    return false;
}

bool isEligibleThresholdActuator(const ActuatorSlotConfiguration& slot) {
    if (!slot.enabled || slot.implementation == ActuatorImplementation::None) return false;
    const ActuatorImplementationMetadata* metadata =
        ActuatorImplementationRegistry::find(slot.implementation);
    return metadata != nullptr
        && hasActuatorCapability(metadata->capabilities, ActuatorCapability::OnOff);
}

bool isControllerTargetClaimedByOtherEnabledSlot(
    const ControllerSlotConfiguration* slots,
    size_t slotCount,
    ControllerId editedSlotId,
    ActuatorId targetActuatorId) {
    if (slots == nullptr || !isValidActuatorId(targetActuatorId)) return false;
    for (size_t index = 0; index < slotCount; ++index) {
        const ControllerSlotConfiguration& slot = slots[index];
        if (slot.slotId == editedSlotId
            || !slot.enabled
            || slot.implementation == ControllerImplementation::None) {
            continue;
        }
        ActuatorId configuredTarget = InvalidActuatorId;
        if (configuredControllerTargetActuatorId(slot, configuredTarget)
            && configuredTarget == targetActuatorId) {
            return true;
        }
    }
    return false;
}

bool applyThresholdControllerWebFields(
    const String& sourceSensor,
    const String& measurementType,
    const String& targetActuator,
    const String& onThreshold,
    const String& offThreshold,
    const String& maxMeasurementAge,
    ControllerSlotConfiguration& slot) {
    uint32_t sourceId = 0;
    uint32_t targetId = 0;
    uint32_t maximumAge = 0;
    float on = 0.0F;
    float off = 0.0F;
    const MeasurementType type = measurementTypeFromStableId(measurementType.c_str());
    if (!parseUnsignedId(sourceSensor, MaxSensorSlotCount, sourceId)
        || !isSupportedMeasurementType(type)
        || !parseUnsignedId(targetActuator, MaxActuatorSlotCount, targetId)
        || !parseFiniteFloat(onThreshold, on)
        || !parseFiniteFloat(offThreshold, off)
        || !parseUnsignedId(maxMeasurementAge, INT32_MAX, maximumAge)) {
        return false;
    }
    ThresholdControllerConfiguration& threshold =
        slot.implementationConfiguration.threshold;
    threshold.source = MeasurementSourceReference(
        static_cast<SensorId>(sourceId), type);
    threshold.targetActuatorId = static_cast<ActuatorId>(targetId);
    threshold.onThreshold = on;
    threshold.offThreshold = off;
    threshold.maxMeasurementAgeMs = maximumAge;
    return true;
}

} // namespace EnvNode
