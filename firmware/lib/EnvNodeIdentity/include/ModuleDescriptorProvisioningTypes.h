// EnvNode: see repository LICENSE.
#pragma once
#include "ModuleSlot.h"
#include "HardwareDescriptorCodec.h"
#include "HardwareDescriptorStore.h"
#include "HardwareDescriptorCompatibilityTypes.h"
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

const char* moduleDescriptorProvisioningStatusName(ModuleDescriptorProvisioningStatus status);
}
