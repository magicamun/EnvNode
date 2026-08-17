#pragma once

#include <cstdint>

namespace EnvNode {

using ControllerId = uint16_t;

constexpr ControllerId InvalidControllerId = 0;

inline bool isValidControllerId(ControllerId id) {
    return id != InvalidControllerId;
}

} // namespace EnvNode
