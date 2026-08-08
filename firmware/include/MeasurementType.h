#pragma once

#include "MeasurementValue.h"
#include "PresentationUnit.h"

namespace WeatherStation {

enum class MeasurementType : uint8_t {
    Unknown,
    Temperature,
    RelativeHumidity,
    AtmosphericPressure,
    SolarIrradiance,
    SolarCellTemperature,
    RainDetectorLevel,
    RainDetectorWet,
    RainGaugeTip,
    RainfallIncrement,
};

enum class MeasurementSemantics : uint8_t {
    Unknown,
    State,
    Event,
};

struct MeasurementTypeMetadata {
    MeasurementType type;
    const char* displayName;
    ValueKind expectedValueKind;
    PresentationUnit canonicalUnit;
    uint8_t recommendedDisplayPrecision;
    MeasurementSemantics semantics;
    const PresentationUnit* supportedPresentationUnits;
    uint8_t supportedPresentationUnitCount;
    PresentationUnit defaultPresentationUnit;
};

bool isSupportedMeasurementType(MeasurementType type);
const MeasurementTypeMetadata& measurementTypeMetadata(MeasurementType type);
bool supportsPresentationUnit(MeasurementType type, PresentationUnit unit);

} // namespace WeatherStation
