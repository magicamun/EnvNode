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

#include "ControllerRuntime.h"
#include "LocaleFormatter.h"
namespace EnvNode {
namespace {
const PropertyEnumOption RainStates[] = {
    {"unknown", "Unknown"}, {"active", "Rain active"}, {"end_estimated", "Rain end estimated"}
};
const char* const RainKeys[] = {"rain", "valid", "heating", "heater_percent", "dry", "wetting_count", "running", "target_available", "rain_start", "rain_end", "rain_end_detected"};
}
const char* const* RainDetectorPropertyReader::keys(size_t& count) {
    count = sizeof(RainKeys)/sizeof(RainKeys[0]); return RainKeys;
}
bool RainDetectorPropertyReader::describe(const PropertyReference& reference, PropertyDescription& result) const {
    result = PropertyDescription{};
    if (reference.componentKind != PropertyComponentKind::Controller || reference.componentId != info_.id
        || info_.implementation != ControllerImplementation::RainDetector || !reference.propertyKey) return false;
    size_t count; const auto list=keys(count);
    for (size_t i=0;i<count;++i) {
        if (strcmp(reference.propertyKey,list[i])) continue;
        result.stableKey=list[i]; result.displayName=list[i];
        result.valueKind=PropertyValueKind::Boolean;
        if (!strcmp(list[i],"rain_start") || !strcmp(list[i],"rain_end") || !strcmp(list[i],"rain_end_detected")) {
            result.valueKind=PropertyValueKind::Text;
            result.displayName = !strcmp(list[i],"rain_start") ? "Last rain start"
                : !strcmp(list[i],"rain_end") ? "Last wetting / estimated rain end" : "Rain end detected at";
        } else if (!strcmp(list[i],"rain")) {
            result.valueKind=PropertyValueKind::Enumeration;
            result.enumOptions=RainStates; result.enumOptionCount=3;
        } else if (!strcmp(list[i],"wetting_count") || !strcmp(list[i],"heater_percent")) {
            result.valueKind=PropertyValueKind::UnsignedInteger;
            if (!strcmp(list[i],"heater_percent")) result.canonicalUnit=PresentationUnit::Percent;
        }
        return true;
    }
    return false;
}
PropertyReadResult RainDetectorPropertyReader::read(const PropertyReference& reference, PropertySnapshot& result) const {
    result=PropertySnapshot{};
    PropertyDescription description;
    if (!describe(reference,description)) return PropertyReadResult::UnknownReference;
    const char* key=reference.propertyKey;
    if (description.valueKind == PropertyValueKind::Text) {
        const auto& event = !strcmp(key,"rain_start") ? info_.rainStart
            : !strcmp(key,"rain_end") ? info_.rainEnd : info_.rainEndDetected;
        if (!event.occurred || !event.hasTimestamp) return PropertyReadResult::NoValue;
        tm local{};
        if (!localtime_r(&event.timestamp,&local)) return PropertyReadResult::NoValue;
        char iso[24];
        if (!strftime(iso,sizeof(iso),"%Y-%m-%d %H:%M:%S",&local)) return PropertyReadResult::NoValue;
        result.value=PropertyValue::text(locale_ ? locale_->formatDateTime(local) : String(iso));
        result.valid=true; result.hasTimestamp=true; result.timestamp=event.timestamp;
        return PropertyReadResult::Available;
    }
    if (!strcmp(key,"rain")) result.value=PropertyValue::enumeration(RainStates[static_cast<unsigned>(info_.rainDetector.rain)]);
    else if (!strcmp(key,"wetting_count")) result.value=MeasurementValue::unsignedInteger(info_.rainWettingCount);
    else if (!strcmp(key,"heater_percent")) result.value=MeasurementValue::unsignedInteger(info_.rainDetector.heating?100:0);
    else {
        const bool value=!strcmp(key,"valid")?info_.rainDetector.valid
            :!strcmp(key,"heating")?info_.rainDetector.heating
            :!strcmp(key,"dry")?info_.rainDetector.dry
            :!strcmp(key,"running")?info_.running:info_.targetAvailable;
        result.value=MeasurementValue::boolean(value);
    }
    // Controller status is available even if the input measurement is invalid.
    result.valid=true;
    return PropertyReadResult::Available;
}
}
