#pragma once

namespace EnvNode {

enum class MeasurementQuality {
    Good,
    Estimated,
    Degraded,
    BelowMeasurementRange,
};

const char* measurementQualityStableId(MeasurementQuality quality);
const char* measurementQualityDisplayName(MeasurementQuality quality);

} // namespace EnvNode
