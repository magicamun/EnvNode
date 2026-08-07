#pragma once

#include "MeasurementValue.h"

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
    const char* canonicalUnit;
};

bool isSupportedMeasurementType(MeasurementType type);
const MeasurementTypeMetadata& measurementTypeMetadata(MeasurementType type);

} // namespace WeatherStation
