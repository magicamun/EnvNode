#pragma once

#include <variant>
#include <ctime>
#include "MeasurementType.h"
#include "MeasurementQuality.h"

namespace WeatherStation {

using MeasurementValue = std::variant<float, bool, uint32_t>;

using SensorId = uint16_t;

struct Measurement {
    MeasurementType type = MeasurementType::Unknown;
    SensorId source = 0;
    std::time_t timestamp = 0;
    MeasurementValue value;
    bool valid = false;
    MeasurementQuality quality = MeasurementQuality::Good;
    bool simulated = false;
};

} // namespace WeatherStation
