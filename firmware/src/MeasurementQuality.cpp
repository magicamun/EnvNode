#include "MeasurementQuality.h"

namespace EnvNode {

const char* measurementQualityStableId(MeasurementQuality quality) {
    switch (quality) {
        case MeasurementQuality::Good: return "good";
        case MeasurementQuality::Estimated: return "estimated";
        case MeasurementQuality::Degraded: return "degraded";
        case MeasurementQuality::BelowMeasurementRange: return "below_measurement_range";
        default: return "degraded";
    }
}

const char* measurementQualityDisplayName(MeasurementQuality quality) {
    switch (quality) {
        case MeasurementQuality::Good: return "Good";
        case MeasurementQuality::Estimated: return "Estimated";
        case MeasurementQuality::Degraded: return "Degraded";
        case MeasurementQuality::BelowMeasurementRange: return "Below measurement range";
        default: return "Unknown";
    }
}

} // namespace EnvNode
