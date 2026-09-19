#include "ModuleDiscoveryService.h"

namespace EnvNode {
namespace {

size_t slotIndex(ModuleSlot slot) {
    return static_cast<size_t>(slot);
}

} // namespace

bool ModuleDiscoveryResult::identified() const {
    if (source == ModuleDiscoverySource::Descriptor) {
        return descriptorStatus == HardwareDescriptorDecodeStatus::Valid;
    }
    return source == ModuleDiscoverySource::LegacyEmidV1
        && (status == ModuleIdentityStatus::Valid
            || status == ModuleIdentityStatus::UnassignedSerial);
}

ModuleDiscoveryService::ModuleDiscoveryService(
    ModuleIdentityStore& slotAStore,
    ModuleIdentityStore& slotBStore)
    : stores_{&slotAStore, &slotBStore}
    , results_{{ModuleSlot::A, SlotAEepromAddress},
               {ModuleSlot::B, SlotBEepromAddress}} {
}

ModuleDiscoveryService::ModuleDiscoveryService(
    ModuleIdentityStore& slotAStore,
    ModuleIdentityStore& slotBStore,
    HardwareDescriptorStore& slotADescriptorStore,
    HardwareDescriptorStore& slotBDescriptorStore)
    : stores_{&slotAStore, &slotBStore}
    , descriptorStores_{&slotADescriptorStore, &slotBDescriptorStore}
    , results_{{ModuleSlot::A, SlotAEepromAddress},
               {ModuleSlot::B, SlotBEepromAddress}} {
}

void ModuleDiscoveryService::scan() {
    for (size_t index = 0; index < SlotCount; ++index) {
        results_[index].source = ModuleDiscoverySource::None;
        results_[index].status = ModuleIdentityStatus::StorageUnavailable;
        results_[index].identity = {};
        results_[index].profile = nullptr;
        results_[index].descriptorStoreStatus =
            HardwareDescriptorStoreStatus::NotProvisioned;
        results_[index].descriptorStatus =
            HardwareDescriptorDecodeStatus::InvalidCbor;
        results_[index].descriptorCompatibility =
            HardwareDescriptorCompatibilityStatus::UnsupportedPlatform;
        results_[index].descriptorTypeId = {};
        results_[index].descriptorName = {};
        results_[index].descriptorRevision = {};
        results_[index].descriptorSerialNumber = {};
        if (descriptorStores_[index] != nullptr) {
            const HardwareDescriptorReadResult descriptorRead =
                descriptorStores_[index]->read(
                    descriptorPayloads_[index], sizeof(descriptorPayloads_[index]));
            results_[index].descriptorStoreStatus = descriptorRead.status;
            if (descriptorRead.status == HardwareDescriptorStoreStatus::Valid) {
                results_[index].source = ModuleDiscoverySource::Descriptor;
                results_[index].descriptorStatus = HardwareDescriptorCodec::decode(
                    descriptorPayloads_[index], descriptorRead.envelope.payloadLength,
                    HardwareDescriptorObjectKind::Module, descriptorScratch_);
                if (results_[index].descriptorStatus == HardwareDescriptorDecodeStatus::Valid) {
                    results_[index].descriptorTypeId = descriptorScratch_.typeId;
                    results_[index].descriptorName = descriptorScratch_.name;
                    results_[index].descriptorRevision = descriptorScratch_.hardwareRevision;
                    results_[index].descriptorSerialNumber = descriptorScratch_.serialNumber;
                    results_[index].descriptorCompatibility =
                        evaluateModuleDescriptorCompatibility(
                            descriptorScratch_, currentFirmwareDescriptorVersion(),
                            currentBoardProfile(), results_[index].slot).status;
                }
                continue;
            }
            if (descriptorRead.status != HardwareDescriptorStoreStatus::NotProvisioned) {
                results_[index].source = ModuleDiscoverySource::Descriptor;
                continue;
            }
        }
        const ModuleIdentityReadResult readResult = stores_[index]->read();
        results_[index].status = readResult.status;
        results_[index].identity = readResult.identity;
        results_[index].profile = results_[index].identified()
            ? ModuleProfileRegistry::find(
                readResult.identity.profileId, readResult.identity.revision)
            : nullptr;
        if (readResult.status == ModuleIdentityStatus::Valid
            || readResult.status == ModuleIdentityStatus::UnassignedSerial) {
            results_[index].source = ModuleDiscoverySource::LegacyEmidV1;
        }
    }
}

const char* moduleDiscoverySourceName(ModuleDiscoverySource source) {
    switch (source) {
        case ModuleDiscoverySource::Descriptor: return "Descriptor";
        case ModuleDiscoverySource::LegacyEmidV1: return "LegacyEmidV1";
        case ModuleDiscoverySource::None: return "None";
        default: return "None";
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
