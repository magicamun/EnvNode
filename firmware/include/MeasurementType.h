#pragma once

#include "MeasurementValue.h"
#include "PresentationUnit.h"

namespace WeatherStation {

enum class MeasurementType {
    Unknown,
    Temperature,
    RelativeHumidity,
    AtmosphericPressure,
    SolarIrradiance,
    SolarCellTemperature,
    RainDetectorLevel,
    RainDetectorWet,
    RainGaugeTip,
};

struct MeasurementTypeMetadata {
    ValueKind expectedValueKind;
    PresentationUnit canonicalUnit;
    const PresentationUnit* supportedPresentationUnits;
    uint8_t supportedPresentationUnitCount;
    PresentationUnit defaultPresentationUnit;
};

bool isSupportedMeasurementType(MeasurementType type);
const MeasurementTypeMetadata& measurementTypeMetadata(MeasurementType type);
bool supportsPresentationUnit(MeasurementType type, PresentationUnit unit);

} // namespace WeatherStation
