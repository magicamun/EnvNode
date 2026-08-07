#include "Measurement.h"

namespace WeatherStation {
namespace {

const PresentationUnit NoUnit[] = {PresentationUnit::None};
const PresentationUnit TemperatureUnits[] = {
    PresentationUnit::DegreeCelsius,
    PresentationUnit::DegreeFahrenheit,
};
const PresentationUnit HumidityUnits[] = {PresentationUnit::Percent};
const PresentationUnit PressureUnits[] = {
    PresentationUnit::Pascal,
    PresentationUnit::Hectopascal,
    PresentationUnit::Kilopascal,
    PresentationUnit::InchMercury,
};
const PresentationUnit IrradianceUnits[] = {PresentationUnit::WattPerSquareMetre};
const PresentationUnit RainLevelUnits[] = {
    PresentationUnit::Ratio,
    PresentationUnit::Percent,
};

template <size_t Size>
constexpr uint8_t unitCount(const PresentationUnit (&)[Size]) {
    return static_cast<uint8_t>(Size);
}

const MeasurementTypeMetadata UnknownMetadata = {
    ValueKind::None, PresentationUnit::None, NoUnit, unitCount(NoUnit), PresentationUnit::None};
const MeasurementTypeMetadata TemperatureMetadata = {
    ValueKind::FloatingPoint, PresentationUnit::DegreeCelsius,
    TemperatureUnits, unitCount(TemperatureUnits), PresentationUnit::DegreeCelsius};
const MeasurementTypeMetadata RelativeHumidityMetadata = {
    ValueKind::FloatingPoint, PresentationUnit::Percent,
    HumidityUnits, unitCount(HumidityUnits), PresentationUnit::Percent};
const MeasurementTypeMetadata AtmosphericPressureMetadata = {
    ValueKind::FloatingPoint, PresentationUnit::Pascal,
    PressureUnits, unitCount(PressureUnits), PresentationUnit::Pascal};
const MeasurementTypeMetadata SolarIrradianceMetadata = {
    ValueKind::FloatingPoint, PresentationUnit::WattPerSquareMetre,
    IrradianceUnits, unitCount(IrradianceUnits), PresentationUnit::WattPerSquareMetre};
const MeasurementTypeMetadata SolarCellTemperatureMetadata = {
    ValueKind::FloatingPoint, PresentationUnit::DegreeCelsius,
    TemperatureUnits, unitCount(TemperatureUnits), PresentationUnit::DegreeCelsius};
const MeasurementTypeMetadata RainDetectorLevelMetadata = {
    ValueKind::FloatingPoint, PresentationUnit::Ratio,
    RainLevelUnits, unitCount(RainLevelUnits), PresentationUnit::Ratio};
const MeasurementTypeMetadata RainDetectorWetMetadata = {
    ValueKind::Boolean, PresentationUnit::None, NoUnit, unitCount(NoUnit), PresentationUnit::None};
const MeasurementTypeMetadata RainGaugeTipMetadata = {
    ValueKind::None, PresentationUnit::None, NoUnit, unitCount(NoUnit), PresentationUnit::None};

bool hasStructurallyValidPayload(const Measurement& measurement) {
    if (!isSupportedMeasurementType(measurement.type)) {
        return false;
    }

    const ValueKind expectedKind = measurementTypeMetadata(measurement.type).expectedValueKind;

    if (!measurement.valid) {
        return measurement.value.kind() == ValueKind::None;
    }

    return measurement.value.kind() == expectedKind;
}

} // namespace

bool isSupportedMeasurementType(MeasurementType type) {
    switch (type) {
        case MeasurementType::Temperature:
        case MeasurementType::RelativeHumidity:
        case MeasurementType::AtmosphericPressure:
        case MeasurementType::SolarIrradiance:
        case MeasurementType::SolarCellTemperature:
        case MeasurementType::RainDetectorLevel:
        case MeasurementType::RainDetectorWet:
        case MeasurementType::RainGaugeTip:
            return true;
        case MeasurementType::Unknown:
        default:
            return false;
    }
}

const MeasurementTypeMetadata& measurementTypeMetadata(MeasurementType type) {
    switch (type) {
        case MeasurementType::Temperature:
            return TemperatureMetadata;
        case MeasurementType::RelativeHumidity:
            return RelativeHumidityMetadata;
        case MeasurementType::AtmosphericPressure:
            return AtmosphericPressureMetadata;
        case MeasurementType::SolarIrradiance:
            return SolarIrradianceMetadata;
        case MeasurementType::SolarCellTemperature:
            return SolarCellTemperatureMetadata;
        case MeasurementType::RainDetectorLevel:
            return RainDetectorLevelMetadata;
        case MeasurementType::RainDetectorWet:
            return RainDetectorWetMetadata;
        case MeasurementType::RainGaugeTip:
            return RainGaugeTipMetadata;
        case MeasurementType::Unknown:
        default:
            return UnknownMetadata;
    }
}

bool supportsPresentationUnit(MeasurementType type, PresentationUnit unit) {
    if (!isSupportedMeasurementType(type)) {
        return false;
    }

    const MeasurementTypeMetadata& metadata = measurementTypeMetadata(type);
    for (uint8_t index = 0; index < metadata.supportedPresentationUnitCount; ++index) {
        if (metadata.supportedPresentationUnits[index] == unit) {
            return true;
        }
    }
    return false;
}

bool isMeasurementContentStructurallyValid(const Measurement& measurement) {
    return hasStructurallyValidPayload(measurement)
        && measurement.source == InvalidSensorId
        && measurement.timestamp == 0;
}

bool isMeasurementStructurallyValid(const Measurement& measurement) {
    return hasStructurallyValidPayload(measurement)
        && isValidSensorId(measurement.source)
        && measurement.timestamp > 0;
}

} // namespace WeatherStation
