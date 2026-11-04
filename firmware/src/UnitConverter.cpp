#include "UnitConverter.h"

#include <string.h>

namespace EnvNode {

PresentationUnit UnitConverter::resolvePresentationUnit(MeasurementType type, PresentationUnit configured) {
    return supportsPresentationUnit(type, configured)
        ? configured : measurementTypeMetadata(type).defaultPresentationUnit;
}

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
        case PresentationUnit::Kilohertz:
            presentationValue = canonicalValue / 1000.0F;
            return type == MeasurementType::RainDetectorFrequency;
        case PresentationUnit::DegreeFahrenheit:
            presentationValue = canonicalValue * (9.0F / 5.0F) + 32.0F;
            return type == MeasurementType::Temperature
                || type == MeasurementType::SolarCellTemperature
                || type == MeasurementType::DewPoint;
        case PresentationUnit::Hectopascal:
            presentationValue = canonicalValue / 100.0F;
            return type == MeasurementType::SeaLevelPressure
                || type == MeasurementType::AtmosphericPressure
                || type == MeasurementType::HydrostaticPressure;
        case PresentationUnit::Kilopascal:
            presentationValue = canonicalValue / 1000.0F;
            return type == MeasurementType::SeaLevelPressure
                || type == MeasurementType::AtmosphericPressure
                || type == MeasurementType::HydrostaticPressure;
        case PresentationUnit::InchMercury:
            presentationValue = canonicalValue / 3386.389F;
            return type == MeasurementType::SeaLevelPressure
                || type == MeasurementType::AtmosphericPressure;
        case PresentationUnit::Percent:
            presentationValue = type == MeasurementType::RainDetectorLevel
                ? canonicalValue * 100.0F
                : canonicalValue;
            return true;
        case PresentationUnit::Centimetre:
            presentationValue = canonicalValue * 100.0F;
            return type == MeasurementType::WaterLevel;
        case PresentationUnit::Millimeter:
            presentationValue = type == MeasurementType::WaterLevel
                ? canonicalValue * 1000.0F
                : canonicalValue;
            return true;
        case PresentationUnit::GramPerCubicMetre:
        case PresentationUnit::Degree:
        case PresentationUnit::DegreeCelsius:
        case PresentationUnit::Pascal:
        case PresentationUnit::WattPerSquareMetre:
        case PresentationUnit::Ratio:
        case PresentationUnit::Metre:
        case PresentationUnit::Hertz:
            presentationValue = canonicalValue;
            return true;
        case PresentationUnit::None:
        default:
            return false;
    }
}

const char* UnitConverter::symbol(PresentationUnit unit) {
    switch (unit) {
        case PresentationUnit::GramPerCubicMetre: return "g/m\xC2\xB3";
        case PresentationUnit::Degree: return "\xC2\xB0";

        case PresentationUnit::Kilohertz: return "kHz";
        case PresentationUnit::Hertz: return "Hz";
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
        case PresentationUnit::Metre: return "m";
        case PresentationUnit::Centimetre: return "cm";
        case PresentationUnit::None:
        default: return nullptr;
    }
}

const char* UnitConverter::displayName(PresentationUnit unit) {
    switch (unit) {
        case PresentationUnit::GramPerCubicMetre: return "Gram per cubic metre";
        case PresentationUnit::Degree: return "Degree";

        case PresentationUnit::Kilohertz: return "Kilohertz";
        case PresentationUnit::Hertz: return "Hertz";
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
        case PresentationUnit::Metre: return "Metre";
        case PresentationUnit::Centimetre: return "Centimetre";
        case PresentationUnit::None: return "None";
        default: return "Unknown";
    }
}

const char* UnitConverter::stableKey(PresentationUnit unit) {
    switch (unit) {
        case PresentationUnit::GramPerCubicMetre: return "gram_per_cubic_metre";
        case PresentationUnit::Degree: return "degree";

        case PresentationUnit::Kilohertz: return "kilohertz";
        case PresentationUnit::Hertz: return "hertz";
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
        case PresentationUnit::Metre: return "metre";
        case PresentationUnit::Centimetre: return "centimetre";
        default: return nullptr;
    }
}

bool UnitConverter::parseStableKey(const char* key, PresentationUnit& unit) {
    if (key == nullptr) {
        return false;
    }
    for (uint8_t value = static_cast<uint8_t>(PresentationUnit::None);
         value <= static_cast<uint8_t>(PresentationUnit::Kilohertz);
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
