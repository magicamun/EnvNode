#pragma once

#include <cstdint>

namespace EnvNode {

using ActuatorId = uint16_t;

constexpr ActuatorId InvalidActuatorId = 0;

inline bool isValidActuatorId(ActuatorId id) {
    return id != InvalidActuatorId;
}

} // namespace EnvNode
