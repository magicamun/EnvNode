#pragma once

#include <ctime>
#include "MeasurementValue.h"
#include "MeasurementType.h"
#include "MeasurementQuality.h"
#include "SensorId.h"
#include "SensorProvenance.h"

namespace EnvNode {

struct Measurement {
    MeasurementType type = MeasurementType::Unknown;
    SensorId source = InvalidSensorId;
    std::time_t timestamp = 0;
    MeasurementValue value;
    bool valid = false;
    MeasurementQuality quality = MeasurementQuality::Good;
    SensorProvenance provenance = SensorProvenance::Physical;
};

bool isMeasurementContentStructurallyValid(const Measurement& measurement);
bool isMeasurementStructurallyValid(const Measurement& measurement);

} // namespace EnvNode
