#pragma once

#include "MeasurementValue.h"
#include "PresentationUnit.h"

namespace EnvNode {

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

constexpr uint8_t SupportedMeasurementTypeCount =
    static_cast<uint8_t>(MeasurementType::RainfallIncrement);

enum class MeasurementSemantics : uint8_t {
    Unknown,
    State,
    Event,
};

struct MeasurementTypeMetadata {
    MeasurementType type;
    const char* stableId;
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
const char* measurementTypeStableId(MeasurementType type);
MeasurementType measurementTypeFromStableId(const char* stableId);
bool supportsPresentationUnit(MeasurementType type, PresentationUnit unit);

} // namespace EnvNode
