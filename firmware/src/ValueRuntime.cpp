#include "ValueRuntime.h"

namespace EnvNode {
const EnumValueConfiguration* ValueRuntime::valueDefinition(ValueId id) const {
    const auto* value = find(id);
    return value ? &value->configuration() : nullptr;
}
bool ValueRuntime::valueCode(ValueId id, String& code) const {
    code = String();
    const auto* value = find(id);
    if (!value || !value->current()) return false;
    code = value->current()->code;
    return true;
}
void ValueRuntime::begin() {
    ++compositionRevision_;
    values_.clear();
    for (const auto& stored : configuration_.getConfiguration().values) {
        EnumValue value(stored.definition);
        if (value.begin(stored.savedCode) != EnumValueStartResult::InvalidConfiguration)
            values_.push_back(value);
    }
}
const EnumValue* ValueRuntime::find(ValueId id) const {
    for (const auto& value : values_) if (value.configuration().id == id) return &value;
    return nullptr;
}
ValueCommandResult ValueRuntime::set(ValueId id, const String& code) {
    for (auto& value : values_) {
        if (value.configuration().id != id) continue;
        EnumValue candidate = value;
        const auto result = candidate.set(code);
        if (result == EnumValueSetResult::UnknownOption || result == EnumValueSetResult::NotStarted)
            return ValueCommandResult::UnknownOption;
        // Also checkpoint an unchanged default on the first persistent command.
        if (value.configuration().restartPolicy == ValueRestartPolicy::RestoreLastValue
            && !configuration_.saveEnumValueCode(id, code)) return ValueCommandResult::StorageFailed;
        if (result == EnumValueSetResult::Unchanged) return ValueCommandResult::Unchanged;
        value = candidate;
        return ValueCommandResult::Changed;
    }
    return ValueCommandResult::UnknownValue;
}
bool ValueRuntime::saveDefinition(const EnumValueConfiguration& definition, bool create) {
    if (!validEnumValueConfiguration(definition) || (find(definition.id) == nullptr) != create) return false;
    std::vector<EnumValueConfiguration> definitions;
    for (const auto& stored : configuration_.getConfiguration().values)
        definitions.push_back(stored.definition.id == definition.id ? definition : stored.definition);
    if (create) definitions.push_back(definition);
    if (!configuration_.setEnumValueDefinitions(definitions)) return false;
    ++compositionRevision_;
    for (const auto& stored : configuration_.getConfiguration().values) {
        if (stored.definition.id != definition.id) continue;
        EnumValue replacement(stored.definition);
        replacement.begin(stored.savedCode);
        for (auto& value : values_) {
            if (value.configuration().id == definition.id) { value = replacement; return true; }
        }
        values_.push_back(replacement);
        return true;
    }
    return false;
}
bool ValueRuntime::remove(ValueId id) {
    if (find(id) == nullptr) return false;
    std::vector<EnumValueConfiguration> definitions;
    for (const auto& stored : configuration_.getConfiguration().values)
        if (stored.definition.id != id) definitions.push_back(stored.definition);
    if (!configuration_.setEnumValueDefinitions(definitions)) return false;
    ++compositionRevision_;
    for (auto it = values_.begin(); it != values_.end(); ++it)
        if (it->configuration().id == id) { values_.erase(it); break; }
    return true;
}
} // namespace EnvNode
