#pragma once

#include <stdint.h>

namespace EnvNode {

enum class Locale : uint8_t {
    GermanGermany = 0,
    EnglishUnitedKingdom = 1,
    EnglishUnitedStates = 2,
};

const char* localeKey(Locale locale);
bool parseLocaleKey(const char* key, Locale& locale);

} // namespace EnvNode
