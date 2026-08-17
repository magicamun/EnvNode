#pragma once

#include "MeasurementType.h"
#include "SensorId.h"

namespace EnvNode {

struct MeasurementSourceReference {
    MeasurementSourceReference(
        SensorId sourceSensorId = InvalidSensorId,
        MeasurementType sourceMeasurementType = MeasurementType::Unknown)
        : sensorId(sourceSensorId)
        , measurementType(sourceMeasurementType) {
    }

    SensorId sensorId = InvalidSensorId;
    MeasurementType measurementType = MeasurementType::Unknown;
};

inline bool operator==(
    const MeasurementSourceReference& left,
    const MeasurementSourceReference& right) {
    return left.sensorId == right.sensorId
        && left.measurementType == right.measurementType;
}

inline bool operator!=(
    const MeasurementSourceReference& left,
    const MeasurementSourceReference& right) {
    return !(left == right);
}

} // namespace EnvNode
