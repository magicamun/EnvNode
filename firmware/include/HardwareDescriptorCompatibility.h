#pragma once

#include "BoardProfile.h"
#include "HardwareDescriptor.h"

namespace EnvNode {

enum class HardwareDescriptorCompatibilityStatus : uint8_t {
    Compatible,
    FirmwareTooOld,
    UnsupportedPlatform,
    UnsupportedSafetyProfile,
    UnsupportedInterface,
    MissingDriver,
    MissingCapability,
    SlotUnavailable,
    ResourceUnavailable,
    ResourceKindMismatch,
    ResourceCapabilityMismatch,
};

struct HardwareDescriptorCompatibilityResult {
    HardwareDescriptorCompatibilityStatus status =
        HardwareDescriptorCompatibilityStatus::UnsupportedPlatform;
    size_t failedIndex = 0;
};

HardwareDescriptorCompatibilityResult evaluateModuleDescriptorCompatibility(
    const HardwareDescriptor& descriptor,
    DescriptorSemanticVersion firmwareVersion,
    const BoardProfile& board,
    ModuleSlot slot);

DescriptorSemanticVersion currentFirmwareDescriptorVersion();

const char* hardwareDescriptorCompatibilityStatusName(
    HardwareDescriptorCompatibilityStatus status);

} // namespace EnvNode
