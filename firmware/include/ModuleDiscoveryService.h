#pragma once

#include "ModuleSlot.h"
#include "HardwareDescriptorCodec.h"
#include "HardwareDescriptorCompatibility.h"
#include "HardwareDescriptorStore.h"

namespace EnvNode {

enum class ModuleDiscoverySource : uint8_t {
    None,
    Descriptor,
};

struct ModuleDiscoveryResult {
    ModuleDiscoveryResult(
        ModuleSlot slotId = ModuleSlot::A,
        uint8_t address = 0)
        : slot(slotId), eepromAddress(address) {
    }

    ModuleSlot slot = ModuleSlot::A;
    uint8_t eepromAddress = 0;
    ModuleDiscoverySource source = ModuleDiscoverySource::None;
    HardwareDescriptorStoreStatus descriptorStoreStatus =
        HardwareDescriptorStoreStatus::NotProvisioned;
    HardwareDescriptorDecodeStatus descriptorStatus =
        HardwareDescriptorDecodeStatus::InvalidCbor;
    HardwareDescriptorCompatibilityStatus descriptorCompatibility =
        HardwareDescriptorCompatibilityStatus::UnsupportedPlatform;
    DescriptorTextView descriptorTypeId = {};
    DescriptorTextView descriptorName = {};
    DescriptorHardwareRevision descriptorRevision = {};
    uint8_t descriptorInstanceId[16] = {};
    bool descriptorHasInstanceId = false;
    DescriptorTextView descriptorSerialNumber = {};
    DescriptorTextView descriptorProductionBatch = {};
    DescriptorTextView descriptorProductionDate = {};
    HardwareDescriptor descriptor = {};

    bool identified() const;
};

class ModuleDiscoveryService {
public:
    static constexpr size_t SlotCount = 2;
    static constexpr uint8_t SlotAEepromAddress = 0x52;
    static constexpr uint8_t SlotBEepromAddress = 0x53;

    ModuleDiscoveryService(
        HardwareDescriptorStore& slotADescriptorStore,
        HardwareDescriptorStore& slotBDescriptorStore);

    void scan();
    const ModuleDiscoveryResult* result(ModuleSlot slot) const;

private:
    HardwareDescriptorStore* descriptorStores_[SlotCount] = {};
    uint8_t descriptorPayloads_[SlotCount][HardwareDescriptorStore::MaximumPayloadSize] = {};
    HardwareDescriptor descriptorScratch_ = {};
    ModuleDiscoveryResult results_[SlotCount];
};

const char* moduleDiscoverySourceName(ModuleDiscoverySource source);

} // namespace EnvNode
