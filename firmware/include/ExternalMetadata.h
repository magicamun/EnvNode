#pragma once

#include "ActuatorImplementationRegistry.h"
#include "MeasurementType.h"
#include "MeasurementValue.h"

namespace EnvNode {

enum class ControllerRuntimeCommand {
    Start,
    Stop,
};

const char* actuatorCapabilityStableName(ActuatorCapability capability);
const char* measurementSemanticsStableName(MeasurementSemantics semantics);
const char* measurementValueKindStableName(ValueKind valueKind);
const char* controllerRuntimeCommandStableName(ControllerRuntimeCommand command);

} // namespace EnvNode
