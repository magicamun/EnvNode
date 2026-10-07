#include "ActuatorPropertyReader.h"
#include <cstring>

namespace EnvNode {

ActuatorPropertyReader::ActuatorPropertyReader(ActuatorId id, const IOnOffActuator& actuator, const ILevelActuator* level)
    : id_(id), actuator_(actuator), level_(level) {}

bool ActuatorPropertyReader::matches(const PropertyReference& reference) const {
    return reference.componentKind == PropertyComponentKind::Actuator
        && isValidActuatorId(id_) && reference.componentId == id_
        && reference.propertyKey != nullptr
        && (strcmp(reference.propertyKey, "state") == 0
            || (level_ != nullptr && strcmp(reference.propertyKey, "level") == 0));
}

bool ActuatorPropertyReader::describe(
    const PropertyReference& reference, PropertyDescription& result) const {
    result = PropertyDescription{};
    if (!matches(reference)) return false;
    if (strcmp(reference.propertyKey, "level") == 0) {
        result.stableKey = "level";
        result.displayName = "Logical output level";
        result.valueKind = PropertyValueKind::UnsignedInteger;
        result.canonicalUnit = PresentationUnit::Percent;
        return true;
    }
    result.stableKey = "state";
    result.displayName = "Logical output state";
    result.valueKind = PropertyValueKind::Boolean;
    result.trueText = "On";
    result.falseText = "Off";
    return true;
}

PropertyReadResult ActuatorPropertyReader::read(
    const PropertyReference& reference, PropertySnapshot& result) const {
    result = PropertySnapshot{};
    if (!matches(reference)) return PropertyReadResult::UnknownReference;
    if (!actuator_.initialized()) return PropertyReadResult::NoValue;
    result.value = strcmp(reference.propertyKey, "level") == 0
        ? MeasurementValue::unsignedInteger(level_->level().percent())
        : MeasurementValue::boolean(actuator_.state() == OnOffState::On);
    result.valid = true;
    // The capability has no sample/transition time, quality or revision.
    return PropertyReadResult::Available;
}

} // namespace EnvNode
