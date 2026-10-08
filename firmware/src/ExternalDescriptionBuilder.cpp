#include "ExternalDescriptionBuilder.h"

#include <cstdio>

#include "ControllerParameterMetadata.h"
#include "ExternalMetadata.h"
#include "JsonWriter.h"
#include "UnitConverter.h"

namespace EnvNode {
namespace {

void appendJsonName(String& output, const char* name) {
    appendJsonString(output, name);
    output += ':';
}

void appendJsonBoolean(String& output, bool value) {
    output += value ? "true" : "false";
}

void appendJsonUnsigned(String& output, uint32_t value) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(value));
    output += buffer;
}

void appendJsonFloat(String& output, float value) {
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%.9g", static_cast<double>(value));
    output += buffer;
}

void appendCapabilities(String& output, ActuatorCapability capabilities) {
    output += '[';
    bool appended = false;
    if (hasActuatorCapability(capabilities, ActuatorCapability::OnOff)) {
        appendJsonString(output,
            actuatorCapabilityStableName(ActuatorCapability::OnOff));
        appended = true;
    }
    if (hasActuatorCapability(capabilities, ActuatorCapability::Level)) {
        if (appended) output += ',';
        appendJsonString(output,
            actuatorCapabilityStableName(ActuatorCapability::Level));
    }
    output += ']';
}

const char* parameterUnit(
    const ControllerParameterDescriptor& descriptor,
    MeasurementType sourceType) {
    if (descriptor.unit == ControllerParameterUnit::Milliseconds) return "ms";
    if (descriptor.unit == ControllerParameterUnit::None) return "";
    const PresentationUnit unit = measurementTypeMetadata(sourceType).canonicalUnit;
    return UnitConverter::stableKey(unit);
}

void appendParameterValue(
    String& output,
    const ControllerSlotConfiguration& slot,
    ControllerParameter parameter) {
    if (slot.implementation == ControllerImplementation::Blink) {
        const BlinkControllerConfiguration& blink =
            slot.implementationConfiguration.blink;
        appendJsonUnsigned(output,
            parameter == ControllerParameter::OnDurationMs
                ? blink.onDurationMs : blink.offDurationMs);
        return;
    }
    const ThresholdControllerConfiguration& threshold =
        slot.implementationConfiguration.threshold;
    switch (parameter) {
        case ControllerParameter::OnThreshold:
            appendJsonFloat(output, threshold.onThreshold);
            break;
        case ControllerParameter::OffThreshold:
            appendJsonFloat(output, threshold.offThreshold);
            break;
        case ControllerParameter::ThresholdDirection:
            appendJsonString(output, thresholdDirectionStableName(threshold.direction));
            break;
        case ControllerParameter::MaxMeasurementAgeMs:
            appendJsonUnsigned(output, threshold.maxMeasurementAgeMs);
            break;
        default:
            output += '0';
            break;
    }
}

void appendParameters(
    String& output,
    const ControllerSlotConfiguration& slot) {
    size_t count = 0;
    const ControllerParameterDescriptor* descriptors =
        controllerParameterDescriptors(slot.implementation, count);
    output += '[';
    for (size_t index = 0; index < count; ++index) {
        if (index != 0) output += ',';
        const ControllerParameterDescriptor& descriptor = descriptors[index];
        output += '{';
        appendJsonName(output, "name");
        appendJsonString(output, descriptor.stableName);
        output += ',';
        appendJsonName(output, "display_name");
        appendJsonString(output, descriptor.displayName);
        output += ',';
        appendJsonName(output, "type");
        appendJsonString(output,
            controllerParameterValueTypeStableName(descriptor.valueType));
        output += ',';
        appendJsonName(output, "unit");
        appendJsonString(output, parameterUnit(
            descriptor,
            slot.implementationConfiguration.threshold.source.measurementType));
        output += ',';
        appendJsonName(output, "writable");
        appendJsonBoolean(output, descriptor.writable);
        if (descriptor.hasMinimum) {
            output += ',';
            appendJsonName(output, "minimum");
            appendJsonUnsigned(output, descriptor.minimum);
        }
        if (descriptor.hasMaximum) {
            output += ',';
            appendJsonName(output, "maximum");
            appendJsonUnsigned(output, descriptor.maximum);
        }
        output += ',';
        appendJsonName(output, "value");
        appendParameterValue(output, slot, descriptor.parameter);
        output += '}';
    }
    output += ']';
}

} // namespace

bool buildActuatorDescription(
    const ActuatorSlotConfiguration& slot,
    String& output) {
    output = String();
    if (slot.implementation == ActuatorImplementation::None) return false;
    const ActuatorImplementationMetadata* metadata =
        ActuatorImplementationRegistry::find(slot.implementation);
    if (metadata == nullptr) return false;

    output.reserve(256 + slot.name.length());
    output += '{';
    appendJsonName(output, "schema");
    output += '1';
    output += ',';
    appendJsonName(output, "id");
    appendJsonUnsigned(output, slot.slotId);
    output += ',';
    appendJsonName(output, "name");
    appendJsonString(output, slot.name.c_str());
    output += ',';
    appendJsonName(output, "enabled");
    appendJsonBoolean(output, slot.enabled);
    output += ',';
    appendJsonName(output, "implementation");
    appendJsonString(output, metadata->stableId);
    output += ',';
    appendJsonName(output, "implementation_name");
    appendJsonString(output, metadata->displayType);
    output += ',';
    appendJsonName(output, "capabilities");
    appendCapabilities(output, metadata->capabilities);
    output += '}';
    return true;
}

bool buildControllerDescription(
    const ControllerSlotConfiguration& slot,
    String& output) {
    output = String();
    if (slot.implementation == ControllerImplementation::None) return false;
    const ControllerImplementationMetadata* metadata =
        ControllerImplementationRegistry::find(slot.implementation);
    if (metadata == nullptr) return false;

    output.reserve(slot.implementation == ControllerImplementation::Threshold
        ? 1200 : 850);
    output += '{';
    appendJsonName(output, "schema");
    output += '1';
    output += ',';
    appendJsonName(output, "id");
    appendJsonUnsigned(output, slot.slotId);
    output += ',';
    appendJsonName(output, "name");
    appendJsonString(output, slot.name.c_str());
    output += ',';
    appendJsonName(output, "enabled");
    appendJsonBoolean(output, slot.enabled);
    output += ',';
    appendJsonName(output, "implementation");
    appendJsonString(output, metadata->stableId);
    output += ',';
    appendJsonName(output, "implementation_name");
    appendJsonString(output, metadata->displayType);
    output += ',';
    appendJsonName(output, "runtime_commands");
    output += '[';
    appendJsonString(output,
        controllerRuntimeCommandStableName(ControllerRuntimeCommand::Start));
    output += ',';
    appendJsonString(output,
        controllerRuntimeCommandStableName(ControllerRuntimeCommand::Stop));
    output += ']';
    output += ',';
    appendJsonName(output, "target");
    output += '{';
    appendJsonName(output, "actuator_id");
    ActuatorId target = InvalidActuatorId;
    configuredControllerTargetActuatorId(slot, target);
    appendJsonUnsigned(output, target);
    output += ',';
    appendJsonName(output, "required_capabilities");
    appendCapabilities(output, slot.implementation == ControllerImplementation::Threshold
        && slot.implementationConfiguration.threshold.decisionOnly
        ? ActuatorCapability::None : metadata->requiredActuatorCapabilities);
    output += '}';
    output += ',';
    appendJsonName(output, "inputs");
    output += '[';
    if (slot.implementation == ControllerImplementation::Selector) {
        const auto& selector = slot.implementationConfiguration.selector;
        output += "{\"value_id\":"; appendJsonUnsigned(output, selector.modeValueId);
        output += ",\"automatic_controller_id\":"; appendJsonUnsigned(output, selector.automaticControllerId);
        output += ",\"automatic_code\":"; appendJsonString(output, selector.automaticCode.c_str());
        output += ",\"on_code\":"; appendJsonString(output, selector.onCode.c_str());
        output += ",\"off_code\":"; appendJsonString(output, selector.offCode.c_str());
        output += ",\"unknown_behavior\":\"hold\"}";
    }
    if (slot.implementation == ControllerImplementation::Threshold) {
        const MeasurementSourceReference& source =
            slot.implementationConfiguration.threshold.source;
        const MeasurementTypeMetadata& measurement =
            measurementTypeMetadata(source.measurementType);
        output += '{';
        appendJsonName(output, "sensor_id");
        appendJsonUnsigned(output, source.sensorId);
        output += ',';
        appendJsonName(output, "measurement");
        appendJsonString(output, measurement.stableId);
        output += ',';
        appendJsonName(output, "measurement_name");
        appendJsonString(output, measurement.displayName);
        output += ',';
        appendJsonName(output, "value_type");
        appendJsonString(output,
            measurementValueKindStableName(measurement.expectedValueKind));
        output += ',';
        appendJsonName(output, "semantics");
        appendJsonString(output,
            measurementSemanticsStableName(measurement.semantics));
        output += ',';
        appendJsonName(output, "unit");
        appendJsonString(output, UnitConverter::stableKey(measurement.canonicalUnit));
        output += '}';
    }
    output += ']';
    output += ',';
    appendJsonName(output, "parameters");
    appendParameters(output, slot);
    output += '}';
    return true;
}

} // namespace EnvNode
