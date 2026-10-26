// EnvNode: see repository LICENSE.
#pragma once
#include <cstddef>
#include <cstdint>
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

const char* hardwareDescriptorCompatibilityStatusName(HardwareDescriptorCompatibilityStatus status);
}
