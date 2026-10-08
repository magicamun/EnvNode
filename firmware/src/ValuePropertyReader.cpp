#include "ValuePropertyReader.h"
#include <cstring>

namespace EnvNode {
const EnumValue* ValuePropertyReader::resolve(const PropertyReference& reference) const {
    if (reference.componentKind != PropertyComponentKind::Value || !reference.propertyKey
        || strcmp(reference.propertyKey, "state") != 0) return nullptr;
    return runtime_.find(reference.componentId);
}
bool ValuePropertyReader::describe(const PropertyReference& reference, PropertyDescription& result) const {
    result = PropertyDescription{};
    const auto* value = resolve(reference);
    if (!value) return false;
    auto metadata = std::make_shared<PropertyEnumMetadata>();
    for (const auto& option : value->configuration().options) {
        metadata->codes.push_back(option.code);
        metadata->labels.push_back(option.label);
    }
    for (size_t i = 0; i < metadata->codes.size(); ++i)
        metadata->options.push_back({metadata->codes[i].c_str(), metadata->labels[i].c_str()});
    result.stableKey = "state";
    result.displayName = "Value state";
    result.valueKind = PropertyValueKind::Enumeration;
    result.enumOptions = metadata->options.data();
    result.enumOptionCount = metadata->options.size();
    result.enumMetadata = metadata;
    return true;
}
PropertyReadResult ValuePropertyReader::read(const PropertyReference& reference, PropertySnapshot& result) const {
    result = PropertySnapshot{};
    const auto* value = resolve(reference);
    if (!value) return PropertyReadResult::UnknownReference;
    const auto* current = value->current();
    if (!current) return PropertyReadResult::NoValue;
    result.value = PropertyValue::enumeration(current->code, current->label);
    result.valid = true;
    result.hasRevision = true;
    result.revision = value->revision();
    return PropertyReadResult::Available;
}
} // namespace EnvNode
