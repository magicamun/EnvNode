#include "UnitConverter.h"

#include <string.h>

namespace EnvNode {

bool UnitConverter::convert(
    MeasurementType type,
    float canonicalValue,
    PresentationUnit presentationUnit,
    float& presentationValue) {
    if (!supportsPresentationUnit(type, presentationUnit)
        || measurementTypeMetadata(type).expectedValueKind != ValueKind::FloatingPoint) {
        return false;
    }

    switch (presentationUnit) {
        case PresentationUnit::DegreeFahrenheit:
            presentationValue = canonicalValue * (9.0F / 5.0F) + 32.0F;
            return type == MeasurementType::Temperature
                || type == MeasurementType::SolarCellTemperature;
        case PresentationUnit::Hectopascal:
            presentationValue = canonicalValue / 100.0F;
            return type == MeasurementType::AtmosphericPressure;
        case PresentationUnit::Kilopascal:
            presentationValue = canonicalValue / 1000.0F;
            return type == MeasurementType::AtmosphericPressure;
        case PresentationUnit::InchMercury:
            presentationValue = canonicalValue / 3386.389F;
            return type == MeasurementType::AtmosphericPressure;
        case PresentationUnit::Percent:
            presentationValue = type == MeasurementType::RainDetectorLevel
                ? canonicalValue * 100.0F
                : canonicalValue;
            return true;
        case PresentationUnit::DegreeCelsius:
        case PresentationUnit::Pascal:
        case PresentationUnit::WattPerSquareMetre:
        case PresentationUnit::Ratio:
        case PresentationUnit::Millimeter:
            presentationValue = canonicalValue;
            return true;
        case PresentationUnit::None:
        default:
            return false;
    }
}

const char* UnitConverter::symbol(PresentationUnit unit) {
    switch (unit) {
        case PresentationUnit::DegreeCelsius: return "\xC2\xB0" "C";
        case PresentationUnit::DegreeFahrenheit: return "\xC2\xB0" "F";
        case PresentationUnit::Pascal: return "Pa";
        case PresentationUnit::Hectopascal: return "hPa";
        case PresentationUnit::Kilopascal: return "kPa";
        case PresentationUnit::InchMercury: return "inHg";
        case PresentationUnit::Percent: return "%";
        case PresentationUnit::WattPerSquareMetre: return "W/m\xC2\xB2";
        case PresentationUnit::Ratio: return "ratio";
        case PresentationUnit::Millimeter: return "mm";
        case PresentationUnit::None:
        default: return nullptr;
    }
}

const char* UnitConverter::displayName(PresentationUnit unit) {
    switch (unit) {
        case PresentationUnit::DegreeCelsius: return "Celsius";
        case PresentationUnit::DegreeFahrenheit: return "Fahrenheit";
        case PresentationUnit::Pascal: return "Pascal";
        case PresentationUnit::Hectopascal: return "hPa";
        case PresentationUnit::Kilopascal: return "kPa";
        case PresentationUnit::InchMercury: return "inHg";
        case PresentationUnit::Percent: return "Percent";
        case PresentationUnit::WattPerSquareMetre: return "Watt per square metre";
        case PresentationUnit::Ratio: return "Ratio";
        case PresentationUnit::Millimeter: return "Millimetre";
        case PresentationUnit::None: return "None";
        default: return "Unknown";
    }
}

const char* UnitConverter::stableKey(PresentationUnit unit) {
    switch (unit) {
        case PresentationUnit::None: return "none";
        case PresentationUnit::DegreeCelsius: return "degree_celsius";
        case PresentationUnit::DegreeFahrenheit: return "degree_fahrenheit";
        case PresentationUnit::Pascal: return "pascal";
        case PresentationUnit::Hectopascal: return "hectopascal";
        case PresentationUnit::Kilopascal: return "kilopascal";
        case PresentationUnit::InchMercury: return "inch_mercury";
        case PresentationUnit::Percent: return "percent";
        case PresentationUnit::WattPerSquareMetre: return "watt_per_square_metre";
        case PresentationUnit::Ratio: return "ratio";
        case PresentationUnit::Millimeter: return "millimeter";
        default: return nullptr;
    }
}

bool UnitConverter::parseStableKey(const char* key, PresentationUnit& unit) {
    if (key == nullptr) {
        return false;
    }
    for (uint8_t value = static_cast<uint8_t>(PresentationUnit::None);
         value <= static_cast<uint8_t>(PresentationUnit::Millimeter);
         ++value) {
        const PresentationUnit candidate = static_cast<PresentationUnit>(value);
        const char* candidateKey = stableKey(candidate);
        if (candidateKey != nullptr && strcmp(key, candidateKey) == 0) {
            unit = candidate;
            return true;
        }
    }
    return false;
}

} // namespace EnvNode
