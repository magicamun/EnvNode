#pragma once

#include "HardwareDescriptorCompatibility.h"
#include "ModuleDescriptorProvisioningTypes.h"
#include "HardwareDescriptorStore.h"
#include "ModuleDiscoveryService.h"

namespace EnvNode {

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
};


} // namespace EnvNode
