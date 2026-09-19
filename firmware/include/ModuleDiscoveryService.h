#pragma once

#include "ModuleIdentityStore.h"
#include "ModuleProfile.h"
#include "ModuleSlot.h"

namespace EnvNode {

struct ModuleDiscoveryResult {
    ModuleDiscoveryResult(
        ModuleSlot slotId = ModuleSlot::A,
        uint8_t address = 0)
        : slot(slotId), eepromAddress(address) {
    }

    ModuleSlot slot = ModuleSlot::A;
    uint8_t eepromAddress = 0;
    ModuleIdentityStatus status = ModuleIdentityStatus::StorageUnavailable;
    ModuleIdentity identity = {};
    const ModuleProfile* profile = nullptr;

    bool identified() const;
};

class ModuleDiscoveryService {
public:
    // Legacy EMID v1 discovery for the two physical slots of the current
    // EnvNode Mini. It is diagnostic only and never activates a driver.
    static constexpr size_t SlotCount = 2;
    static constexpr uint8_t SlotAEepromAddress = 0x52;
    static constexpr uint8_t SlotBEepromAddress = 0x53;

    ModuleDiscoveryService(
        ModuleIdentityStore& slotAStore,
        ModuleIdentityStore& slotBStore);

    void scan();
    const ModuleDiscoveryResult* result(ModuleSlot slot) const;

private:
    ModuleIdentityStore* stores_[SlotCount];
    ModuleDiscoveryResult results_[SlotCount];
};

const char* moduleIdentityStatusName(ModuleIdentityStatus status);

} // namespace EnvNode
