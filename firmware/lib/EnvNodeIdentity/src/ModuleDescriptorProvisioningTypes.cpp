// EnvNode: see repository LICENSE.
#include "ModuleDescriptorProvisioningTypes.h"
namespace EnvNode {
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
}
