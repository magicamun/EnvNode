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
        const char* canonical = localeKey(candidate);
        bool matches = strlen(key) == strlen(canonical);
        for (size_t i = 0; matches && canonical[i]; ++i) {
            const char input = key[i] == '_' ? '-' : key[i];
            const char foldedInput = input >= 'A' && input <= 'Z' ? input + ('a' - 'A') : input;
            const char foldedCanonical = canonical[i] >= 'A' && canonical[i] <= 'Z'
                ? canonical[i] + ('a' - 'A') : canonical[i];
            matches = foldedInput == foldedCanonical;
        }
        if (matches) {
            locale = candidate;
            return true;
        }
    }
    return false;
}

} // namespace EnvNode
