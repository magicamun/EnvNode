#include "ModuleDescriptorProvisioningService.h"

#include "BoardProfile.h"
#include "ModuleDescriptorWriter.h"
#include <memory>
#include <new>

namespace EnvNode {
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
    std::unique_ptr<HardwareDescriptor> scratch(new (std::nothrow) HardwareDescriptor);
    if (!scratch) {
        result.status = ModuleDescriptorProvisioningStatus::StorageUnavailable;
        return result;
    }
    result = writeModuleProvisioningPayload(*selectedStore, slot, payload, payloadSize,
        explicitlyConfirmed, *scratch);
    if (result.status != ModuleDescriptorProvisioningStatus::Success) return result;
    result.compatibilityStatus = evaluateModuleDescriptorCompatibility(
        *scratch, currentFirmwareDescriptorVersion(), currentBoardProfile(), slot).status;

    scratch.reset();
    discoveryService_.scan();
    const ModuleDiscoveryResult* discovered = discoveryService_.result(slot);
    if (discovered == nullptr
        || discovered->source != ModuleDiscoverySource::Descriptor
        || discovered->descriptorStatus != HardwareDescriptorDecodeStatus::Valid) {
        result.status = ModuleDescriptorProvisioningStatus::RediscoveryFailed;
        return result;
    }
    result.status = ModuleDescriptorProvisioningStatus::Success;
    return result;
}



} // namespace EnvNode
