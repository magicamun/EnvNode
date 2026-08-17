#include "ControllerParameterMetadata.h"

#include <climits>

namespace EnvNode {
namespace {

const ControllerParameterDescriptor BlinkParameters[] = {
    {ControllerParameter::OnDurationMs, "on_duration_ms", "On duration",
        ControllerParameterValueType::UnsignedInteger,
        ControllerParameterUnit::Milliseconds, true, true, 1, true, INT32_MAX},
    {ControllerParameter::OffDurationMs, "off_duration_ms", "Off duration",
        ControllerParameterValueType::UnsignedInteger,
        ControllerParameterUnit::Milliseconds, true, true, 1, true, INT32_MAX},
};

const ControllerParameterDescriptor ThresholdParameters[] = {
    {ControllerParameter::OnThreshold, "on_threshold", "On threshold",
        ControllerParameterValueType::FloatingPoint,
        ControllerParameterUnit::SourceMeasurementCanonical,
        true, false, 0, false, 0},
    {ControllerParameter::OffThreshold, "off_threshold", "Off threshold",
        ControllerParameterValueType::FloatingPoint,
        ControllerParameterUnit::SourceMeasurementCanonical,
        true, false, 0, false, 0},
    {ControllerParameter::MaxMeasurementAgeMs, "max_measurement_age_ms",
        "Maximum Measurement age", ControllerParameterValueType::UnsignedInteger,
        ControllerParameterUnit::Milliseconds, true, true, 1, true, INT32_MAX},
};

template <size_t Size>
constexpr size_t arraySize(const ControllerParameterDescriptor (&)[Size]) {
    return Size;
}

} // namespace

const char* controllerParameterValueTypeStableName(
    ControllerParameterValueType valueType) {
    switch (valueType) {
        case ControllerParameterValueType::UnsignedInteger: return "uint32";
        case ControllerParameterValueType::FloatingPoint: return "float";
        default: return nullptr;
    }
}

const ControllerParameterDescriptor* controllerParameterDescriptor(
    ControllerParameter parameter) {
    for (const ControllerParameterDescriptor& descriptor : BlinkParameters) {
        if (descriptor.parameter == parameter) return &descriptor;
    }
    for (const ControllerParameterDescriptor& descriptor : ThresholdParameters) {
        if (descriptor.parameter == parameter) return &descriptor;
    }
    return nullptr;
}

const ControllerParameterDescriptor* controllerParameterDescriptors(
    ControllerImplementation implementation,
    size_t& count) {
    if (implementation == ControllerImplementation::Blink) {
        count = arraySize(BlinkParameters);
        return BlinkParameters;
    }
    if (implementation == ControllerImplementation::Threshold) {
        count = arraySize(ThresholdParameters);
        return ThresholdParameters;
    }
    count = 0;
    return nullptr;
}

} // namespace EnvNode
