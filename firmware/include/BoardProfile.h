#pragma once

#include <cstddef>
#include <cstdint>

#include "HardwareResources.h"
#include "ModuleSlot.h"

namespace EnvNode {

enum class BoardProfileId : uint8_t {
    EnvNodeMainboard = 0,
};

struct BoardRevision {
    uint8_t major;
    uint8_t minor;
};

struct BoardModuleSlotCapability {
    ModuleSlot slot;
    uint8_t identityEepromAddress;
    GpioResource auxGpio1;
    GpioResource auxGpio2;
    GpioResource spiChipSelect;
    bool fiveVoltSupplyAvailable;
};

struct BoardProfile {
    BoardProfileId id;
    const char* displayName;
    BoardRevision revision;
    const BoardGpioCapability* gpios;
    size_t gpioCount;
    const BoardI2CBusCapability* i2cBuses;
    size_t i2cBusCount;
    const BoardModuleSlotCapability* moduleSlots;
    size_t moduleSlotCount;
};

const BoardProfile& currentBoardProfile();
const BoardProfile* boardProfile(BoardProfileId id);
size_t boardProfileCount();
const BoardProfile* boardProfileAt(size_t index);
BoardProfileId buildFallbackBoardProfileId();
bool selectCurrentBoardProfile(BoardProfileId id);
const BoardModuleSlotCapability* boardModuleSlot(
    const BoardProfile& board,
    ModuleSlot slot);

} // namespace EnvNode
