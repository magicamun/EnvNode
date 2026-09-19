#include "ModuleDescriptorProvisioningService.h"

#include "BoardProfile.h"
#include "HardwareDescriptorCodec.h"

namespace EnvNode {
namespace {

bool hasAssignedInstanceId(const HardwareDescriptor& descriptor) {
    for (size_t index = 0; index < sizeof(descriptor.instanceId); ++index) {
        if (descriptor.instanceId[index] != 0) return true;
    }
    return false;
}

} // namespace

ModuleDescriptorProvisioningService::ModuleDescriptorProvisioningService(
    HardwareDescriptorStore& slotAStore,
    HardwareDescriptorStore& slotBStore,
    ModuleDiscoveryService& discoveryService)
    : stores_{&slotAStore, &slotBStore}
    , discoveryService_(discoveryService) {
}

HardwareDescriptorStore* ModuleDescriptorProvisioningService::store(
    ModuleSlot slot) const {
    const size_t index = static_cast<size_t>(slot);
    return index < ModuleDiscoveryService::SlotCount ? stores_[index] : nullptr;
}

ModuleDescriptorProvisioningResult ModuleDescriptorProvisioningService::provision(
    ModuleSlot slot,
    const uint8_t* payload,
    size_t payloadSize,
    bool explicitlyConfirmed) {
    ModuleDescriptorProvisioningResult result;
    result.slot = slot;
    HardwareDescriptorStore* selectedStore = store(slot);
    if (selectedStore == nullptr) {
        result.status = ModuleDescriptorProvisioningStatus::InvalidSlot;
        return result;
    }
    if (!explicitlyConfirmed) {
        result.status = ModuleDescriptorProvisioningStatus::ConfirmationRequired;
        return result;
    }

    result.decodeStatus = HardwareDescriptorCodec::decode(
        payload, payloadSize, HardwareDescriptorObjectKind::Module,
        descriptorScratch_);
    if (result.decodeStatus != HardwareDescriptorDecodeStatus::Valid
        || !hasAssignedInstanceId(descriptorScratch_)) {
        result.status = ModuleDescriptorProvisioningStatus::InvalidDescriptor;
        return result;
    }
    const HardwareDescriptorCompatibilityResult compatibility =
        evaluateModuleDescriptorCompatibility(
            descriptorScratch_, currentFirmwareDescriptorVersion(),
            currentBoardProfile(), slot);
    result.compatibilityStatus = compatibility.status;
    if (compatibility.status != HardwareDescriptorCompatibilityStatus::Compatible) {
        result.status = ModuleDescriptorProvisioningStatus::IncompatibleDescriptor;
        return result;
    }

    const HardwareDescriptorWriteResult write = selectedStore->write(
        HardwareDescriptorObjectKind::Module, payload, payloadSize);
    result.bank = write.bank;
    result.generation = write.generation;
    if (write.status != HardwareDescriptorStoreStatus::Valid) {
        if (write.status == HardwareDescriptorStoreStatus::StorageUnavailable) {
            result.status = ModuleDescriptorProvisioningStatus::StorageUnavailable;
        } else if (write.status == HardwareDescriptorStoreStatus::WriteFailed) {
            result.status = ModuleDescriptorProvisioningStatus::WriteFailed;
        } else {
            result.status = ModuleDescriptorProvisioningStatus::VerificationFailed;
        }
        return result;
    }

    discoveryService_.scan();
    const ModuleDiscoveryResult* discovered = discoveryService_.result(slot);
    if (discovered == nullptr
        || discovered->source != ModuleDiscoverySource::Descriptor
        || discovered->descriptorStatus != HardwareDescriptorDecodeStatus::Valid
        || discovered->descriptorCompatibility
            != HardwareDescriptorCompatibilityStatus::Compatible) {
        result.status = ModuleDescriptorProvisioningStatus::RediscoveryFailed;
        return result;
    }
    result.status = ModuleDescriptorProvisioningStatus::Success;
    return result;
}

const char* moduleDescriptorProvisioningStatusName(
    ModuleDescriptorProvisioningStatus status) {
    switch (status) {
        case ModuleDescriptorProvisioningStatus::Success: return "Success";
        case ModuleDescriptorProvisioningStatus::ConfirmationRequired: return "ConfirmationRequired";
        case ModuleDescriptorProvisioningStatus::InvalidSlot: return "InvalidSlot";
        case ModuleDescriptorProvisioningStatus::InvalidDescriptor: return "InvalidDescriptor";
        case ModuleDescriptorProvisioningStatus::IncompatibleDescriptor: return "IncompatibleDescriptor";
        case ModuleDescriptorProvisioningStatus::StorageUnavailable: return "StorageUnavailable";
        case ModuleDescriptorProvisioningStatus::WriteFailed: return "WriteFailed";
        case ModuleDescriptorProvisioningStatus::VerificationFailed: return "VerificationFailed";
        case ModuleDescriptorProvisioningStatus::RediscoveryFailed: return "RediscoveryFailed";
        default: return "InvalidDescriptor";
    }
}

} // namespace EnvNode
