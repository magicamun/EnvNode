#include "ModuleDiscoveryService.h"

namespace EnvNode {
namespace {

size_t slotIndex(ModuleSlot slot) {
    return static_cast<size_t>(slot);
}

} // namespace

bool ModuleDiscoveryResult::identified() const {
    return status == ModuleIdentityStatus::Valid
        || status == ModuleIdentityStatus::UnassignedSerial;
}

ModuleDiscoveryService::ModuleDiscoveryService(
    ModuleIdentityStore& slotAStore,
    ModuleIdentityStore& slotBStore)
    : stores_{&slotAStore, &slotBStore}
    , results_{{ModuleSlot::A, SlotAEepromAddress},
               {ModuleSlot::B, SlotBEepromAddress}} {
}

void ModuleDiscoveryService::scan() {
    for (size_t index = 0; index < SlotCount; ++index) {
        const ModuleIdentityReadResult readResult = stores_[index]->read();
        results_[index].status = readResult.status;
        results_[index].identity = readResult.identity;
        results_[index].profile = results_[index].identified()
            ? ModuleProfileRegistry::find(
                readResult.identity.profileId, readResult.identity.revision)
            : nullptr;
    }
}

const ModuleDiscoveryResult* ModuleDiscoveryService::result(ModuleSlot slot) const {
    const size_t index = slotIndex(slot);
    return index < SlotCount ? &results_[index] : nullptr;
}

const char* moduleIdentityStatusName(ModuleIdentityStatus status) {
    switch (status) {
        case ModuleIdentityStatus::Valid: return "Valid";
        case ModuleIdentityStatus::StorageUnavailable: return "StorageUnavailable";
        case ModuleIdentityStatus::NotProvisioned: return "NotProvisioned";
        case ModuleIdentityStatus::InvalidMagic: return "InvalidMagic";
        case ModuleIdentityStatus::UnsupportedFormat: return "UnsupportedFormat";
        case ModuleIdentityStatus::InvalidLength: return "InvalidLength";
        case ModuleIdentityStatus::InvalidCRC: return "InvalidCRC";
        case ModuleIdentityStatus::UnknownModuleProfile: return "UnknownModuleProfile";
        case ModuleIdentityStatus::InvalidRevision: return "InvalidRevision";
        case ModuleIdentityStatus::UnsupportedRevision: return "UnsupportedRevision";
        case ModuleIdentityStatus::UnassignedSerial: return "UnassignedSerial";
        default: return "Unknown";
    }
}

} // namespace EnvNode
