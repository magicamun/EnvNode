#pragma once

#include <cstddef>
#include <cstdint>

#include "HardwareResources.h"

namespace EnvNode {

enum class BoardProfileId : uint8_t {
    EnvNodeMainboard,
};

struct BoardRevision {
    uint8_t major;
    uint8_t minor;
};

struct BoardProfile {
    BoardProfileId id;
    const char* displayName;
    BoardRevision revision;
    const BoardGpioCapability* gpios;
    size_t gpioCount;
    const BoardI2CBusCapability* i2cBuses;
    size_t i2cBusCount;
};

const BoardProfile& currentBoardProfile();

} // namespace EnvNode
