#pragma once

#include <cstdint>

namespace EnvNode {

using SensorId = uint16_t;

constexpr SensorId InvalidSensorId = 0;

inline bool isValidSensorId(SensorId id) {
    return id != InvalidSensorId;
}

} // namespace EnvNode
