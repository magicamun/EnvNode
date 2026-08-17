#pragma once

#include <cstddef>
#include <cstdint>

#include "ControllerImplementationRegistry.h"

namespace EnvNode {

enum class ControllerParameter : uint8_t {
    OnDurationMs,
    OffDurationMs,
    OnThreshold,
    OffThreshold,
    MaxMeasurementAgeMs,
    Count,
};

enum class ControllerParameterValueType : uint8_t {
    UnsignedInteger,
    FloatingPoint,
};

enum class ControllerParameterUnit : uint8_t {
    Milliseconds,
    SourceMeasurementCanonical,
};

struct ControllerParameterDescriptor {
    ControllerParameter parameter;
    const char* stableName;
    const char* displayName;
    ControllerParameterValueType valueType;
    ControllerParameterUnit unit;
    bool writable;
    bool hasMinimum;
    uint32_t minimum;
    bool hasMaximum;
    uint32_t maximum;
};

const char* controllerParameterValueTypeStableName(
    ControllerParameterValueType valueType);
const ControllerParameterDescriptor* controllerParameterDescriptor(
    ControllerParameter parameter);
const ControllerParameterDescriptor* controllerParameterDescriptors(
    ControllerImplementation implementation,
    size_t& count);

} // namespace EnvNode
