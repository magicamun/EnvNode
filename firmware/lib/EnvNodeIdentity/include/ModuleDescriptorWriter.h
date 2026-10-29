// EnvNode: see repository LICENSE.
#pragma once
#include "ModuleDescriptorProvisioningTypes.h"
namespace EnvNode {
// Validation and EEPROM writing deliberately have no runtime-driver dependency.
ModuleDescriptorProvisioningResult validateModuleProvisioningPayload(
    ModuleSlot slot, const uint8_t* payload, size_t size, bool confirmed,
    HardwareDescriptor& scratch);
ModuleDescriptorProvisioningResult writeModuleProvisioningPayload(
    HardwareDescriptorStore& store, ModuleSlot slot, const uint8_t* payload,
    size_t size, bool confirmed, HardwareDescriptor& scratch);
}
