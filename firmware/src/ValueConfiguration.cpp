#include "ValueConfiguration.h"
#include <cstring>
#include <string>

namespace EnvNode {
bool validValueConfiguration(const ValueConfiguration& values) {
    if (values.size() > MaxEnumValueCount) return false;
    for (size_t i = 0; i < values.size(); ++i) {
        const auto& value = values[i];
        if (!validEnumValueConfiguration(value.definition)) return false;
        for (size_t j = 0; j < i; ++j)
            if (values[j].definition.id == value.definition.id) return false;
        if (value.savedCode.length() > 0) {
            if (value.definition.restartPolicy != ValueRestartPolicy::RestoreLastValue) return false;
            bool found = false;
            for (const auto& option : value.definition.options)
                if (option.code == value.savedCode) found = true;
            if (!found) return false;
        }
    }
    return true;
}

bool encodeValueConfiguration(const ValueConfiguration& values, String& encoded) {
    if (!validValueConfiguration(values)) return false;
    String result("1\n");
    auto line = [&](const String& value) { result += value.c_str(); result += '\n'; };
    line(String(static_cast<unsigned int>(values.size())));
    for (const auto& value : values) {
        const auto& d = value.definition;
        line(String(d.id)); line(d.name);
        line(d.restartPolicy == ValueRestartPolicy::RestoreLastValue ? "1" : "0");
        line(d.defaultCode); line(value.savedCode);
        line(String(static_cast<unsigned int>(d.options.size())));
        for (const auto& option : d.options) { line(option.code); line(option.label); }
    }
    if (result.length() > MaxValueRecordLength) return false;
    encoded = result;
    return true;
}

bool decodeValueConfiguration(const String& encoded, ValueConfiguration& values) {
    if (encoded.length() > MaxValueRecordLength || strlen(encoded.c_str()) != encoded.length()) return false;
    const std::string input(encoded.c_str());
    size_t position = 0;
    auto line = [&](String& result) {
        const auto end = input.find('\n', position);
        if (end == std::string::npos || end - position > 64) return false;
        result = input.substr(position, end - position).c_str();
        position = end + 1;
        return true;
    };
    auto number = [&](unsigned int maximum, unsigned int& result) {
        String text;
        if (!line(text) || text.length() == 0 || text.length() > 5) return false;
        result = 0;
        for (size_t i = 0; i < text.length(); ++i) {
            if (text[i] < '0' || text[i] > '9') return false;
            result = result * 10 + text[i] - '0';
            if (result > maximum) return false;
        }
        return true;
    };
    String version;
    unsigned int count;
    if (!line(version) || version != "1" || !number(MaxEnumValueCount, count)) return false;
    ValueConfiguration candidate;
    for (unsigned int i = 0; i < count; ++i) {
        StoredEnumValue value;
        unsigned int id, policy, optionCount;
        auto& d = value.definition;
        if (!number(65535, id) || !line(d.name) || !number(1, policy)
            || !line(d.defaultCode) || !line(value.savedCode) || !number(16, optionCount)) return false;
        d.id = static_cast<ValueId>(id);
        d.restartPolicy = static_cast<ValueRestartPolicy>(policy);
        for (unsigned int j = 0; j < optionCount; ++j) {
            EnumValueOption option;
            if (!line(option.code) || !line(option.label)) return false;
            d.options.push_back(option);
        }
        candidate.push_back(value);
    }
    if (position != input.size() || !validValueConfiguration(candidate)) return false;
    values = candidate;
    return true;
}
} // namespace EnvNode
