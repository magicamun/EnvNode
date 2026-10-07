#include "ControllerPropertyReader.h"
#include <cstring>

namespace EnvNode {
namespace {
const PropertyEnumOption Decisions[] = {
    {"unknown", "Unknown"}, {"on", "On"}, {"off", "Off"},
};
constexpr size_t DecisionCount = sizeof(Decisions) / sizeof(Decisions[0]);
static_assert(DecisionCount == static_cast<size_t>(ThresholdDecision::Off) + 1,
    "Each threshold decision needs a stable code and display text");
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
        && reference.propertyKey != nullptr && (strcmp(reference.propertyKey, "reason") == 0
            || strcmp(reference.propertyKey, "decision") == 0);
}

bool ControllerPropertyReader::describe(const PropertyReference& reference, PropertyDescription& result) const {
    result = PropertyDescription{};
    if (!matches(reference)) return false;
    if (strcmp(reference.propertyKey, "decision") == 0) {
        result.stableKey = "decision";
        result.displayName = "Threshold decision";
        result.valueKind = PropertyValueKind::Enumeration;
        result.enumOptions = Decisions;
        result.enumOptionCount = DecisionCount;
        return true;
    }
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
    if (strcmp(reference.propertyKey, "decision") == 0) {
        const auto decision = provider_.decisionCurrent()
            ? provider_.decision() : ThresholdDecision::Unknown;
        const size_t index = static_cast<size_t>(decision);
        if (index >= DecisionCount) return PropertyReadResult::NoValue;
        result.value = PropertyValue::enumeration(Decisions[index]);
        result.valid = true;
        return PropertyReadResult::Available;
    }
    const size_t index = static_cast<size_t>(provider_.reason());
    if (index >= ReasonCount) return PropertyReadResult::NoValue;
    result.value = PropertyValue::enumeration(Reasons[index]);
    result.valid = true;
    return PropertyReadResult::Available;
}

} // namespace EnvNode
