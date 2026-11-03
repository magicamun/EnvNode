#pragma once

#include <cstddef>
#include <cstdint>

#include "BoardIdentityTypes.h"
#include "HardwareResources.h"
#include "ModuleSlot.h"
#include "BoardPortMap.h"

namespace EnvNode {

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
    GpioResource spiClock;
    GpioResource spiMiso;
    GpioResource spiMosi;
};

bool boardPortMapRuntimeSupported(const BoardPortMap&, DescriptorSemanticVersion);
bool selectCurrentBoardPortMap(const BoardPortMap&);
const BoardProfile& currentBoardProfile();
const BoardProfile* boardProfile(BoardProfileId id);
const BoardProfile* boardProfile(BoardProfileId id, BoardRevision revision);
size_t boardProfileCount();
const BoardProfile* boardProfileAt(size_t index);
BoardProfileId buildFallbackBoardProfileId();
bool selectCurrentBoardProfile(BoardProfileId id);
bool selectCurrentBoardProfile(BoardProfileId id, BoardRevision revision);
const BoardModuleSlotCapability* boardModuleSlot(
    const BoardProfile& board,
    ModuleSlot slot);

} // namespace EnvNode
