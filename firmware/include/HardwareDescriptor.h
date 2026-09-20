#pragma once

#include <cstddef>
#include <cstdint>

#include "HardwareDescriptorEnvelope.h"
#include "HardwareDescriptorVocabulary.h"

namespace EnvNode {

struct DescriptorTextView {
    const char* data = nullptr;
    size_t size = 0;

    bool empty() const { return data == nullptr || size == 0; }
    bool equals(const char* value) const;
};

struct DescriptorIdentifier {
    bool coded = false;
    uint16_t code = 0;
    DescriptorTextView text = {};
};

struct DescriptorSemanticVersion {
    DescriptorSemanticVersion(
        uint16_t majorValue = 0,
        uint16_t minorValue = 0,
        uint16_t patchValue = 0)
        : major(majorValue), minor(minorValue), patch(patchValue) {
    }

    uint16_t major = 0;
    uint16_t minor = 0;
    uint16_t patch = 0;
};

struct DescriptorHardwareRevision {
    uint8_t major = 0;
    uint8_t minor = 0;
};

struct DescriptorContract {
    DescriptorIdentifier id = {};
    uint16_t apiVersion = 0;
};

constexpr size_t MaximumDescriptorDrivers = 8;
constexpr size_t MaximumDescriptorCapabilities = 16;
constexpr size_t MaximumDescriptorResources = 32;
constexpr size_t MaximumDescriptorSlots = 4;
constexpr size_t MaximumDescriptorBindings = 12;
constexpr size_t MaximumDescriptorRequirements = 8;
constexpr size_t MaximumDescriptorDevices = 8;
constexpr size_t MaximumDescriptorMeasurements = 8;
constexpr size_t MaximumDescriptorCalibrationEntries = 8;

struct DescriptorCompatibility {
    DescriptorSemanticVersion minimumFirmwareVersion = {};
    DescriptorContract platform = {};
    DescriptorContract safetyProfile = {};
    DescriptorContract drivers[MaximumDescriptorDrivers] = {};
    size_t driverCount = 0;
    DescriptorIdentifier capabilities[MaximumDescriptorCapabilities] = {};
    size_t capabilityCount = 0;
};

struct DescriptorBinding {
    DescriptorTextView name = {};
    DescriptorTextView target = {};
};

struct DescriptorCapabilitySet {
    uint32_t known = 0;
    bool hasUnknown = false;

    bool contains(HardwareDescriptorCapabilityCode capability) const {
        const uint8_t code = static_cast<uint8_t>(capability);
        return code < 32 && (known & (UINT32_C(1) << code)) != 0;
    }
};

struct DescriptorResource {
    DescriptorTextView id = {};
    HardwareDescriptorResourceKind kind = HardwareDescriptorResourceKind::Gpio;
    DescriptorCapabilitySet capabilities = {};
    DescriptorTextView platformBinding = {};
    uint32_t voltageMillivolts = 0;
    bool hasVoltage = false;
};

struct DescriptorSlot {
    DescriptorTextView id = {};
    DescriptorIdentifier interfaceId = {};
    uint8_t identityAddress = 0;
    DescriptorBinding bindings[MaximumDescriptorBindings] = {};
    size_t bindingCount = 0;
};

struct DescriptorRequirement {
    DescriptorTextView id = {};
    DescriptorTextView resource = {};
    HardwareDescriptorResourceKind kind = HardwareDescriptorResourceKind::Gpio;
    DescriptorCapabilitySet capabilities = {};
};

struct DescriptorDevice {
    DescriptorTextView id = {};
    HardwareDescriptorDeviceKind kind = HardwareDescriptorDeviceKind::Sensor;
    DescriptorContract driver = {};
    DescriptorBinding bindings[MaximumDescriptorBindings] = {};
    size_t bindingCount = 0;
    DescriptorIdentifier measurements[MaximumDescriptorMeasurements] = {};
    size_t measurementCount = 0;
    DescriptorCapabilitySet capabilities = {};
    bool hasActiveLevel = false;
    bool activeLevelHigh = false;
    bool hasSafeLevel = false;
    bool safeLevelHigh = false;
};

struct DescriptorCalibrationEntry {
    DescriptorTextView target = {};
    DescriptorContract schema = {};
};

struct HardwareDescriptor {
    HardwareDescriptorObjectKind objectKind = HardwareDescriptorObjectKind::Board;
    DescriptorTextView typeId = {};
    uint8_t instanceId[16] = {};
    DescriptorTextView manufacturer = {};
    DescriptorHardwareRevision hardwareRevision = {};
    DescriptorCompatibility compatibility = {};
    DescriptorTextView name = {};
    DescriptorTextView summary = {};
    DescriptorTextView documentationUrl = {};
    DescriptorIdentifier interfaceId = {};
    DescriptorResource resources[MaximumDescriptorResources] = {};
    size_t resourceCount = 0;
    DescriptorSlot slots[MaximumDescriptorSlots] = {};
    size_t slotCount = 0;
    DescriptorRequirement requirements[MaximumDescriptorRequirements] = {};
    size_t requirementCount = 0;
    DescriptorDevice devices[MaximumDescriptorDevices] = {};
    size_t deviceCount = 0;
    DescriptorTextView serialNumber = {};
    DescriptorTextView productionBatch = {};
    DescriptorTextView productionDate = {};
    DescriptorCalibrationEntry calibration[MaximumDescriptorCalibrationEntries] = {};
    size_t calibrationCount = 0;
};

} // namespace EnvNode
