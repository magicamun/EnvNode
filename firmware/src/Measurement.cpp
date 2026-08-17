#include "Measurement.h"

#include <cstring>

namespace EnvNode {
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
const PresentationUnit RainfallIncrementUnits[] = {PresentationUnit::Millimeter};

template <size_t Size>
constexpr uint8_t unitCount(const PresentationUnit (&)[Size]) {
    return static_cast<uint8_t>(Size);
}

const MeasurementTypeMetadata MeasurementMetadata[] = {
    {MeasurementType::Unknown, "unknown", "Unknown", ValueKind::None, PresentationUnit::None, 0,
        MeasurementSemantics::Unknown, NoUnit, unitCount(NoUnit), PresentationUnit::None},
    {MeasurementType::Temperature, "temperature", "Temperature", ValueKind::FloatingPoint,
        PresentationUnit::DegreeCelsius, 1, MeasurementSemantics::State,
        TemperatureUnits, unitCount(TemperatureUnits), PresentationUnit::DegreeCelsius},
    {MeasurementType::RelativeHumidity, "relative_humidity", "Relative Humidity", ValueKind::FloatingPoint,
        PresentationUnit::Percent, 1, MeasurementSemantics::State,
        HumidityUnits, unitCount(HumidityUnits), PresentationUnit::Percent},
    {MeasurementType::AtmosphericPressure, "atmospheric_pressure", "Atmospheric Pressure", ValueKind::FloatingPoint,
        PresentationUnit::Pascal, 1, MeasurementSemantics::State,
        PressureUnits, unitCount(PressureUnits), PresentationUnit::Pascal},
    {MeasurementType::SolarIrradiance, "solar_irradiance", "Solar Irradiance", ValueKind::FloatingPoint,
        PresentationUnit::WattPerSquareMetre, 1, MeasurementSemantics::State,
        IrradianceUnits, unitCount(IrradianceUnits), PresentationUnit::WattPerSquareMetre},
    {MeasurementType::SolarCellTemperature, "solar_cell_temperature", "Solar Cell Temperature", ValueKind::FloatingPoint,
        PresentationUnit::DegreeCelsius, 1, MeasurementSemantics::State,
        TemperatureUnits, unitCount(TemperatureUnits), PresentationUnit::DegreeCelsius},
    {MeasurementType::RainDetectorLevel, "rain_detector_level", "Rain Detector Level", ValueKind::FloatingPoint,
        PresentationUnit::Ratio, 2, MeasurementSemantics::State,
        RainLevelUnits, unitCount(RainLevelUnits), PresentationUnit::Ratio},
    {MeasurementType::RainDetectorWet, "rain_detector_wet", "Rain Detector Wet", ValueKind::Boolean,
        PresentationUnit::None, 0, MeasurementSemantics::State,
        NoUnit, unitCount(NoUnit), PresentationUnit::None},
    {MeasurementType::RainGaugeTip, "rain_gauge_tip", "Rain Gauge Tip", ValueKind::None,
        PresentationUnit::None, 0, MeasurementSemantics::Event,
        NoUnit, unitCount(NoUnit), PresentationUnit::None},
    {MeasurementType::RainfallIncrement, "rainfall_increment", "Rainfall Increment", ValueKind::FloatingPoint,
        PresentationUnit::Millimeter, 3, MeasurementSemantics::Event,
        RainfallIncrementUnits, unitCount(RainfallIncrementUnits), PresentationUnit::Millimeter},
};

constexpr size_t MeasurementMetadataCount =
    sizeof(MeasurementMetadata) / sizeof(MeasurementMetadata[0]);
static_assert(MeasurementMetadataCount
        == static_cast<size_t>(SupportedMeasurementTypeCount) + 1,
    "Measurement metadata must cover every MeasurementType");

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
    return measurementTypeMetadata(type).type != MeasurementType::Unknown;
}

const MeasurementTypeMetadata& measurementTypeMetadata(MeasurementType type) {
    const size_t index = static_cast<size_t>(type);
    if (index >= MeasurementMetadataCount || MeasurementMetadata[index].type != type) {
        return MeasurementMetadata[0];
    }
    return MeasurementMetadata[index];
}

const char* measurementTypeStableId(MeasurementType type) {
    return measurementTypeMetadata(type).stableId;
}

MeasurementType measurementTypeFromStableId(const char* stableId) {
    if (stableId == nullptr) return MeasurementType::Unknown;
    for (size_t index = 1; index < MeasurementMetadataCount; ++index) {
        if (strcmp(MeasurementMetadata[index].stableId, stableId) == 0) {
            return MeasurementMetadata[index].type;
        }
    }
    return MeasurementType::Unknown;
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

} // namespace EnvNode
