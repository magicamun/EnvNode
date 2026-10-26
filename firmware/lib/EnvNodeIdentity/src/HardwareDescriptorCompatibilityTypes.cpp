// EnvNode: see repository LICENSE.
#include "HardwareDescriptorCompatibilityTypes.h"
namespace EnvNode {
const char* hardwareDescriptorCompatibilityStatusName(
    HardwareDescriptorCompatibilityStatus status) {
    switch (status) {
        case HardwareDescriptorCompatibilityStatus::Compatible: return "Compatible";
        case HardwareDescriptorCompatibilityStatus::FirmwareTooOld: return "FirmwareTooOld";
        case HardwareDescriptorCompatibilityStatus::UnsupportedPlatform: return "UnsupportedPlatform";
        case HardwareDescriptorCompatibilityStatus::UnsupportedSafetyProfile: return "UnsupportedSafetyProfile";
        case HardwareDescriptorCompatibilityStatus::UnsupportedInterface: return "UnsupportedInterface";
        case HardwareDescriptorCompatibilityStatus::MissingDriver: return "MissingDriver";
        case HardwareDescriptorCompatibilityStatus::MissingCapability: return "MissingCapability";
        case HardwareDescriptorCompatibilityStatus::SlotUnavailable: return "SlotUnavailable";
        case HardwareDescriptorCompatibilityStatus::ResourceUnavailable: return "ResourceUnavailable";
        case HardwareDescriptorCompatibilityStatus::ResourceKindMismatch: return "ResourceKindMismatch";
        case HardwareDescriptorCompatibilityStatus::ResourceCapabilityMismatch: return "ResourceCapabilityMismatch";
        default: return "UnsupportedPlatform";
    }
}
}
