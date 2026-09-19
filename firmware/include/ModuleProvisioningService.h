#pragma once

#include "ModuleDiscoveryService.h"

namespace EnvNode {

enum class ModuleProvisioningStatus : uint8_t {
    Success,
    ConfirmationRequired,
    InvalidSlot,
    InvalidIdentity,
    WriteFailed,
    ReadbackFailed,
    ReadbackInvalid,
    ReadbackMismatch,
};

struct ModuleProvisioningResult {
    ModuleProvisioningStatus status = ModuleProvisioningStatus::InvalidIdentity;
    ModuleSlot slot = ModuleSlot::A;
    ModuleIdentity identity = {};
};

class ModuleProvisioningService {
public:
    ModuleProvisioningService(
        ModuleIdentityStore& slotAStore,
        ModuleIdentityStore& slotBStore,
        ModuleDiscoveryService& discoveryService);

    ModuleProvisioningResult provision(
        ModuleSlot slot,
        const ModuleIdentity& identity,
        bool explicitlyConfirmed);

private:
    ModuleIdentityStore* store(ModuleSlot slot) const;

    ModuleIdentityStore* stores_[ModuleDiscoveryService::SlotCount];
    ModuleDiscoveryService& discoveryService_;
};

const char* moduleProvisioningStatusName(ModuleProvisioningStatus status);

} // namespace EnvNode
