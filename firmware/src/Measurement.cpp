#include "Measurement.h"

namespace WeatherStation {
namespace {

const MeasurementTypeMetadata UnknownMetadata = {ValueKind::None, nullptr};
const MeasurementTypeMetadata TemperatureMetadata = {ValueKind::FloatingPoint, "\xC2\xB0" "C"};
const MeasurementTypeMetadata RelativeHumidityMetadata = {ValueKind::FloatingPoint, "%"};
const MeasurementTypeMetadata AtmosphericPressureMetadata = {ValueKind::FloatingPoint, "Pa"};
const MeasurementTypeMetadata SolarIrradianceMetadata = {ValueKind::FloatingPoint, "W/m\xC2\xB2"};
const MeasurementTypeMetadata SolarCellTemperatureMetadata = {ValueKind::FloatingPoint, "\xC2\xB0" "C"};
const MeasurementTypeMetadata RainDetectorLevelMetadata = {ValueKind::FloatingPoint, "ratio"};
const MeasurementTypeMetadata RainDetectorWetMetadata = {ValueKind::Boolean, nullptr};
const MeasurementTypeMetadata RainGaugeTipMetadata = {ValueKind::None, nullptr};

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
