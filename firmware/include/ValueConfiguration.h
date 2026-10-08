#pragma once
#include "EnumValue.h"

namespace EnvNode {
constexpr size_t MaxEnumValueCount = 8;
constexpr size_t MaxValueRecordLength = 14000;
struct StoredEnumValue {
    EnumValueConfiguration definition;
    String savedCode;
};
using ValueConfiguration = std::vector<StoredEnumValue>;
bool validValueConfiguration(const ValueConfiguration& values);
bool encodeValueConfiguration(const ValueConfiguration& values, String& encoded);
bool decodeValueConfiguration(const String& encoded, ValueConfiguration& values);
} // namespace EnvNode
