#include "EnvNode/Locale.h"

#include <string.h>

namespace EnvNode {

const char* localeKey(Locale locale) {
    switch (locale) {
        case Locale::GermanGermany: return "de-DE";
        case Locale::EnglishUnitedKingdom: return "en-GB";
        case Locale::EnglishUnitedStates: return "en-US";
        default: return nullptr;
    }
}

bool parseLocaleKey(const char* key, Locale& locale) {
    if (key == nullptr) return false;
    const Locale supported[] = {
        Locale::GermanGermany,
        Locale::EnglishUnitedKingdom,
        Locale::EnglishUnitedStates,
    };
    for (const Locale candidate : supported) {
        if (strcmp(key, localeKey(candidate)) == 0) {
            locale = candidate;
            return true;
        }
    }
    return false;
}

} // namespace EnvNode
