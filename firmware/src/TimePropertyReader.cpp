#include "TimePropertyReader.h"
#include <cstring>
namespace EnvNode {
bool TimePropertyReader::describe(const PropertyReference& reference, PropertyDescription& result) const {
    result = PropertyDescription{};
    if (reference.componentKind != PropertyComponentKind::System || reference.componentId != 1
        || reference.propertyKey == nullptr) return false;
    if (strcmp(reference.propertyKey, "date") == 0) {
        result.stableKey = "date"; result.displayName = "Local date";
    } else if (strcmp(reference.propertyKey, "time") == 0) {
        result.stableKey = "time"; result.displayName = "Local time";
    } else if (strcmp(reference.propertyKey, "datetime") == 0) {
        result.stableKey = "datetime"; result.displayName = "Local date and time";
    } else return false;
    result.valueKind = PropertyValueKind::Text;
    return true;
}
PropertyReadResult TimePropertyReader::read(const PropertyReference& reference, PropertySnapshot& result) const {
    result = PropertySnapshot{};
    PropertyDescription description;
    if (!describe(reference, description)) return PropertyReadResult::UnknownReference;
    tm local{};
    if (!time_.synchronized() || !time_.localCivilTime(local)) return PropertyReadResult::NoValue;
    const String text = strcmp(description.stableKey, "date") == 0 ? locale_.formatDate(local)
        : strcmp(description.stableKey, "time") == 0 ? locale_.formatTime(local) : locale_.formatDateTime(local);
    result.value = PropertyValue::text(text);
    result.valid = true;
    return PropertyReadResult::Available;
}
}
