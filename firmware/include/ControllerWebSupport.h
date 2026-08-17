#pragma once

#include <Arduino.h>

#include "ActuatorSlotConfiguration.h"
#include "ControllerSlotConfiguration.h"
#include "SensorSlotConfiguration.h"

namespace EnvNode {

bool isThresholdCompatibleMeasurementType(MeasurementType type);
bool sensorSupportsThresholdMeasurement(
    const SensorSlotConfiguration& slot,
    MeasurementType type);
size_t thresholdMeasurementTypesForSensor(
    const SensorSlotConfiguration& slot,
    MeasurementType* types,
    size_t capacity);
bool isEligibleThresholdSensor(const SensorSlotConfiguration& slot);
bool isEligibleThresholdActuator(const ActuatorSlotConfiguration& slot);
bool isControllerTargetClaimedByOtherEnabledSlot(
    const ControllerSlotConfiguration* slots,
    size_t slotCount,
    ControllerId editedSlotId,
    ActuatorId targetActuatorId);
bool applyThresholdControllerWebFields(
    const String& sourceSensor,
    const String& measurementType,
    const String& targetActuator,
    const String& onThreshold,
    const String& offThreshold,
    const String& maxMeasurementAge,
    ControllerSlotConfiguration& slot);

} // namespace EnvNode
