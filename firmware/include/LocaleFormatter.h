#pragma once

#include <Arduino.h>
#include <time.h>

#include "IConfigurationService.h"

namespace WeatherStation {

class LocaleFormatter {
public:
    explicit LocaleFormatter(IConfigurationService& configurationService);

    String formatDateTime(const tm& localTime) const;
    String formatDate(const tm& localTime) const;
    String formatTime(const tm& localTime) const;
    String formatNumber(double value, uint8_t fractionDigits = 1) const;

private:
    Locale locale() const;
    String groupedNumber(const char* normalized, char thousandsSeparator, char decimalSeparator) const;

    IConfigurationService& configurationService_;
};

} // namespace WeatherStation
