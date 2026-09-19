#pragma once

#include "HardwareDescriptorCompatibility.h"
#include "HardwareDescriptorStore.h"
#include "ModuleDiscoveryService.h"

namespace EnvNode {

enum class ModuleDescriptorProvisioningStatus : uint8_t {
    Success,
    ConfirmationRequired,
    InvalidSlot,
    InvalidDescriptor,
    IncompatibleDescriptor,
    StorageUnavailable,
    WriteFailed,
    VerificationFailed,
    RediscoveryFailed,
};

struct ModuleDescriptorProvisioningResult {
    ModuleDescriptorProvisioningStatus status =
        ModuleDescriptorProvisioningStatus::InvalidDescriptor;
    ModuleSlot slot = ModuleSlot::A;
    HardwareDescriptorDecodeStatus decodeStatus =
        HardwareDescriptorDecodeStatus::InvalidCbor;
    HardwareDescriptorCompatibilityStatus compatibilityStatus =
        HardwareDescriptorCompatibilityStatus::UnsupportedPlatform;
    HardwareDescriptorBank bank = HardwareDescriptorBank::None;
    uint32_t generation = 0;
};

class ModuleDescriptorProvisioningService {
public:
    ModuleDescriptorProvisioningService(
        HardwareDescriptorStore& slotAStore,
        HardwareDescriptorStore& slotBStore,
        ModuleDiscoveryService& discoveryService);

    ModuleDescriptorProvisioningResult provision(
        ModuleSlot slot,
        const uint8_t* payload,
        size_t payloadSize,
        bool explicitlyConfirmed);

private:
    HardwareDescriptorStore* store(ModuleSlot slot) const;

    HardwareDescriptorStore* stores_[ModuleDiscoveryService::SlotCount];
    ModuleDiscoveryService& discoveryService_;
    HardwareDescriptor descriptorScratch_ = {};
};

const char* moduleDescriptorProvisioningStatusName(
    ModuleDescriptorProvisioningStatus status);

} // namespace EnvNode
