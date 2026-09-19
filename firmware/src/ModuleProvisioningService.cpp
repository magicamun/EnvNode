#include "ModuleProvisioningService.h"

namespace EnvNode {

ModuleProvisioningService::ModuleProvisioningService(
    ModuleIdentityStore& slotAStore,
    ModuleIdentityStore& slotBStore,
    ModuleDiscoveryService& discoveryService)
    : stores_{&slotAStore, &slotBStore}
    , discoveryService_(discoveryService) {
}

ModuleIdentityStore* ModuleProvisioningService::store(ModuleSlot slot) const {
    const size_t index = static_cast<size_t>(slot);
    return index < ModuleDiscoveryService::SlotCount ? stores_[index] : nullptr;
}

ModuleProvisioningResult ModuleProvisioningService::provision(
    ModuleSlot slot,
    const ModuleIdentity& identity,
    bool explicitlyConfirmed) {
    ModuleProvisioningResult result;
    result.slot = slot;
    result.identity = identity;

    ModuleIdentityStore* selectedStore = store(slot);
    if (selectedStore == nullptr) {
        result.status = ModuleProvisioningStatus::InvalidSlot;
        return result;
    }
    if (!explicitlyConfirmed) {
        result.status = ModuleProvisioningStatus::ConfirmationRequired;
        return result;
    }

    const ModuleIdentityStatus validation = validateModuleIdentity(identity);
    if (validation != ModuleIdentityStatus::Valid
        && validation != ModuleIdentityStatus::UnassignedSerial) {
        result.status = ModuleProvisioningStatus::InvalidIdentity;
        return result;
    }

    switch (selectedStore->write(identity)) {
        case ModuleIdentityWriteStatus::Success:
            result.status = ModuleProvisioningStatus::Success;
            discoveryService_.scan();
            return result;
        case ModuleIdentityWriteStatus::InvalidIdentity:
            result.status = ModuleProvisioningStatus::InvalidIdentity;
            return result;
        case ModuleIdentityWriteStatus::WriteFailed:
            result.status = ModuleProvisioningStatus::WriteFailed;
            return result;
        case ModuleIdentityWriteStatus::ReadbackFailed:
            result.status = ModuleProvisioningStatus::ReadbackFailed;
            return result;
        case ModuleIdentityWriteStatus::ReadbackInvalid:
            result.status = ModuleProvisioningStatus::ReadbackInvalid;
            return result;
        case ModuleIdentityWriteStatus::ReadbackMismatch:
        default:
            result.status = ModuleProvisioningStatus::ReadbackMismatch;
            return result;
    }
}

const char* moduleProvisioningStatusName(ModuleProvisioningStatus status) {
    switch (status) {
        case ModuleProvisioningStatus::Success: return "Success";
        case ModuleProvisioningStatus::ConfirmationRequired: return "ConfirmationRequired";
        case ModuleProvisioningStatus::InvalidSlot: return "InvalidSlot";
        case ModuleProvisioningStatus::InvalidIdentity: return "InvalidIdentity";
        case ModuleProvisioningStatus::WriteFailed: return "WriteFailed";
        case ModuleProvisioningStatus::ReadbackFailed: return "ReadbackFailed";
        case ModuleProvisioningStatus::ReadbackInvalid: return "ReadbackInvalid";
        case ModuleProvisioningStatus::ReadbackMismatch: return "ReadbackMismatch";
        default: return "InvalidIdentity";
    }
}

} // namespace EnvNode
