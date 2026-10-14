// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include "LocalizedDecimal.h"
#include <cstdio>
#include <cmath>
namespace EnvNode {
String formatDecimalInput(double value, unsigned precision, Locale locale) {
    if (!std::isfinite(value) || precision > 9) return String();
    char buffer[64];
    const int length = snprintf(buffer, sizeof(buffer), "%.*f", static_cast<int>(precision), value);
    if (length < 0 || static_cast<size_t>(length) >= sizeof(buffer)) return String();
    String result;
    for (const char* c = buffer; *c; ++c)
        result += *c == '.' && locale == Locale::GermanGermany ? ',' : *c;
    return result;
}
bool normalizeDecimalInput(const String& input, Locale locale, String& canonical) {
    canonical = String();
    if (input.isEmpty()) return true;
    if (input.length() > 32) return false;
    const char separator = locale == Locale::GermanGermany ? ',' : '.';
    String result; bool hasDecimal = false, hasDigit = false;
    for (size_t i = 0; i < input.length(); ++i) {
        const char c = input[i];
        if (i == 0 && (c == '-' || c == '+')) { result += c; continue; }
        if (c == separator && !hasDecimal) {
            hasDecimal = true;
            if (!hasDigit) result += '0';
            result += '.'; continue;
        }
        if (c < '0' || c > '9') return false;
        hasDigit = true; result += c;
    }
    if (!hasDigit) return false;
    canonical = result; return true;
}
}
