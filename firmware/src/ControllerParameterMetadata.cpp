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
    {ControllerParameter::ThresholdDirection, "threshold_direction", "Threshold direction",
        ControllerParameterValueType::String,
        ControllerParameterUnit::None, true, false, 0, false, 0},
    {ControllerParameter::MaxMeasurementAgeMs, "max_measurement_age_ms",
        "Maximum Measurement age", ControllerParameterValueType::UnsignedInteger,
        ControllerParameterUnit::Milliseconds, true, true, 1, true, INT32_MAX},
};

const ControllerParameterDescriptor RainParameters[] = {
    {ControllerParameter::RainEdgeMs,"edge_ms","Maximum edge duration",ControllerParameterValueType::UnsignedInteger,ControllerParameterUnit::Milliseconds,true,true,250,true,2000},
    {ControllerParameter::RainDropHz,"drop_hz","Minimum frequency drop",ControllerParameterValueType::UnsignedInteger,ControllerParameterUnit::Hertz,true,true,20,true,10000},
    {ControllerParameter::RainDropPermille,"drop_permille","Relative frequency drop",ControllerParameterValueType::UnsignedInteger,ControllerParameterUnit::Permille,true,true,1,true,200},
    {ControllerParameter::RainEndSeconds,"end_seconds","Rain-end timeout",ControllerParameterValueType::UnsignedInteger,ControllerParameterUnit::Seconds,true,true,30,true,300},
    {ControllerParameter::RainDryHz,"dry_hz","Dry threshold",ControllerParameterValueType::UnsignedInteger,ControllerParameterUnit::Hertz,true,true,1000,true,100000},
    {ControllerParameter::RainHysteresisHz,"hysteresis_hz","Dry hysteresis",ControllerParameterValueType::UnsignedInteger,ControllerParameterUnit::Hertz,true,true,1,true,99999},
    {ControllerParameter::RainDrySeconds,"dry_seconds","Dry confirmation",ControllerParameterValueType::UnsignedInteger,ControllerParameterUnit::Seconds,true,true,1,true,120},
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
        case ControllerParameterValueType::String: return "string";
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
    for (const auto& descriptor : RainParameters) if (descriptor.parameter == parameter) return &descriptor;
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
    if (implementation == ControllerImplementation::RainDetector) { count=arraySize(RainParameters); return RainParameters; }
    count = 0;
    return nullptr;
}

} // namespace EnvNode
