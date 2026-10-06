#include "ControllerPropertyReader.h"
#include <cstring>

namespace EnvNode {
namespace {
const PropertyEnumOption Reasons[] = {
    {"not_started", "Not started"},
    {"stopped", "Stopped"},
    {"invalid_configuration", "Invalid configuration"},
    {"no_measurement", "No measurement available"},
    {"invalid_measurement", "Invalid or incompatible measurement"},
    {"stale_measurement", "Measurement too old"},
    {"on_threshold", "On threshold reached"},
    {"off_threshold", "Off threshold reached"},
    {"hysteresis_hold", "Holding decision within hysteresis band"},
    {"awaiting_threshold", "Waiting for first threshold decision"},
};
constexpr size_t ReasonCount = sizeof(Reasons) / sizeof(Reasons[0]);
static_assert(ReasonCount == static_cast<size_t>(ThresholdReason::AwaitingThreshold) + 1,
    "Each threshold reason needs a stable code and display text");
}

ControllerPropertyReader::ControllerPropertyReader(ControllerId id, const IThresholdReasonProvider& provider)
    : id_(id), provider_(provider) {}

bool ControllerPropertyReader::matches(const PropertyReference& reference) const {
    return reference.componentKind == PropertyComponentKind::Controller
        && isValidControllerId(id_) && reference.componentId == id_
        && reference.propertyKey != nullptr && strcmp(reference.propertyKey, "reason") == 0;
}

bool ControllerPropertyReader::describe(const PropertyReference& reference, PropertyDescription& result) const {
    result = PropertyDescription{};
    if (!matches(reference)) return false;
    result.stableKey = "reason";
    result.displayName = "Threshold evaluation reason";
    result.valueKind = PropertyValueKind::Enumeration;
    result.enumOptions = Reasons;
    result.enumOptionCount = ReasonCount;
    return true;
}

PropertyReadResult ControllerPropertyReader::read(const PropertyReference& reference, PropertySnapshot& result) const {
    result = PropertySnapshot{};
    if (!matches(reference)) return PropertyReadResult::UnknownReference;
    const size_t index = static_cast<size_t>(provider_.reason());
    if (index >= ReasonCount) return PropertyReadResult::NoValue;
    result.value = PropertyValue::enumeration(Reasons[index]);
    result.valid = true;
    return PropertyReadResult::Available;
}

} // namespace EnvNode
