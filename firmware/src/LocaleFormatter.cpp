#include "LocaleFormatter.h"

#include <stdio.h>
#include <string.h>

namespace WeatherStation {

LocaleFormatter::LocaleFormatter(IConfigurationService& configurationService)
    : configurationService_(configurationService) {
}

Locale LocaleFormatter::locale() const {
    return configurationService_.getLocale();
}

String LocaleFormatter::formatDate(const tm& localTime) const {
    char buffer[16];
    switch (locale()) {
        case Locale::EnglishUnitedStates:
            snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d",
                localTime.tm_mon + 1, localTime.tm_mday, localTime.tm_year + 1900);
            break;
        case Locale::EnglishUnitedKingdom:
            snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d",
                localTime.tm_mday, localTime.tm_mon + 1, localTime.tm_year + 1900);
            break;
        case Locale::GermanGermany:
        default:
            snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d",
                localTime.tm_mday, localTime.tm_mon + 1, localTime.tm_year + 1900);
            break;
    }
    return String(buffer);
}

String LocaleFormatter::formatTime(const tm& localTime) const {
    char buffer[16];
    if (locale() == Locale::EnglishUnitedStates) {
        const int hour12 = localTime.tm_hour % 12 == 0 ? 12 : localTime.tm_hour % 12;
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d %s",
            hour12, localTime.tm_min, localTime.tm_sec, localTime.tm_hour < 12 ? "AM" : "PM");
    } else {
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
            localTime.tm_hour, localTime.tm_min, localTime.tm_sec);
    }
    return String(buffer);
}

String LocaleFormatter::formatDateTime(const tm& localTime) const {
    return formatDate(localTime) + " " + formatTime(localTime);
}

String LocaleFormatter::formatNumber(double value, uint8_t fractionDigits) const {
    if (fractionDigits > 6) fractionDigits = 6;
    char normalized[48];
    snprintf(normalized, sizeof(normalized), "%.*f", fractionDigits, value);
    return locale() == Locale::GermanGermany
        ? groupedNumber(normalized, '.', ',')
        : groupedNumber(normalized, ',', '.');
}

String LocaleFormatter::groupedNumber(
    const char* normalized,
    char thousandsSeparator,
    char decimalSeparator) const {
    const char* decimalPoint = strchr(normalized, '.');
    const size_t integerLength = decimalPoint == nullptr
        ? strlen(normalized) : static_cast<size_t>(decimalPoint - normalized);
    const bool negative = normalized[0] == '-';
    const size_t digitsStart = negative ? 1 : 0;
    const size_t digitCount = integerLength - digitsStart;

    String formatted;
    formatted.reserve(strlen(normalized) + digitCount / 3 + 2);
    if (negative) formatted += '-';
    for (size_t index = digitsStart; index < integerLength; ++index) {
        const size_t digitsRemaining = integerLength - index;
        if (index > digitsStart && digitsRemaining % 3 == 0) formatted += thousandsSeparator;
        formatted += normalized[index];
    }
    if (decimalPoint != nullptr) {
        formatted += decimalSeparator;
        formatted += decimalPoint + 1;
    }
    return formatted;
}

} // namespace WeatherStation
