#include "ExternalMetadata.h"

namespace EnvNode {

const char* actuatorCapabilityStableName(ActuatorCapability capability) {
    return capability == ActuatorCapability::OnOff ? "on_off" : nullptr;
}

const char* measurementSemanticsStableName(MeasurementSemantics semantics) {
    switch (semantics) {
        case MeasurementSemantics::State: return "state";
        case MeasurementSemantics::Event: return "event";
        default: return nullptr;
    }
}

const char* measurementValueKindStableName(ValueKind valueKind) {
    switch (valueKind) {
        case ValueKind::FloatingPoint: return "float";
        case ValueKind::UnsignedInteger: return "uint32";
        case ValueKind::Boolean: return "bool";
        case ValueKind::None: return "none";
        default: return nullptr;
    }
}

const char* controllerRuntimeCommandStableName(ControllerRuntimeCommand command) {
    switch (command) {
        case ControllerRuntimeCommand::Start: return "START";
        case ControllerRuntimeCommand::Stop: return "STOP";
        default: return nullptr;
    }
}

} // namespace EnvNode
