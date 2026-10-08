#include "EnumValue.h"

namespace EnvNode {
namespace {
bool validText(const String& text) {
    if (text.length() == 0 || text.length() > 64) return false;
    for (size_t i = 0; i < text.length(); ++i) {
        const unsigned char c = text[i];
        if (c < 32 || c == 127) return false;
    }
    return true;
}
bool validCode(const String& code) {
    if (code.length() == 0 || code.length() > 32) return false;
    for (size_t i = 0; i < code.length(); ++i) {
        const char c = code[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return false;
    }
    return true;
}
}

bool validEnumValueConfiguration(const EnumValueConfiguration& config) {
    if (config.id == 0 || !validText(config.name)
        || config.options.empty() || config.options.size() > 16
        || (config.restartPolicy != ValueRestartPolicy::DefaultOnRestart
            && config.restartPolicy != ValueRestartPolicy::RestoreLastValue)) return false;
    bool hasDefault = false;
    for (size_t i = 0; i < config.options.size(); ++i) {
        const auto& option = config.options[i];
        if (!validCode(option.code) || !validText(option.label)) return false;
        if (option.code == config.defaultCode) hasDefault = true;
        for (size_t j = 0; j < i; ++j)
            if (config.options[j].code == option.code) return false;
    }
    return hasDefault;
}

EnumValue::EnumValue(const EnumValueConfiguration& configuration)
    : configuration_(configuration) {}

int EnumValue::find(const String& code) const {
    for (size_t i = 0; i < configuration_.options.size(); ++i)
        if (configuration_.options[i].code == code) return static_cast<int>(i);
    return -1;
}

EnumValueStartResult EnumValue::begin(const String& savedCode) {
    selected_ = -1;
    revision_ = 0;
    if (!validEnumValueConfiguration(configuration_)) return EnumValueStartResult::InvalidConfiguration;
    selected_ = find(configuration_.defaultCode);
    if (configuration_.restartPolicy == ValueRestartPolicy::RestoreLastValue && savedCode.length() > 0) {
        const int saved = find(savedCode);
        if (saved < 0) return EnumValueStartResult::SavedValueRejected;
        selected_ = saved;
        return EnumValueStartResult::Restored;
    }
    return EnumValueStartResult::DefaultSelected;
}

EnumValueSetResult EnumValue::set(const String& code) {
    if (selected_ < 0) return EnumValueSetResult::NotStarted;
    const int next = find(code);
    if (next < 0) return EnumValueSetResult::UnknownOption;
    if (next == selected_) return EnumValueSetResult::Unchanged;
    selected_ = next;
    ++revision_;
    return EnumValueSetResult::Changed;
}

const EnumValueOption* EnumValue::current() const {
    return selected_ < 0 ? nullptr : &configuration_.options[selected_];
}
} // namespace EnvNode
