#pragma once

#include "ModuleIdentityStore.h"
#include "ModuleProfile.h"
#include "ModuleSlot.h"
#include "HardwareDescriptorCodec.h"
#include "HardwareDescriptorCompatibility.h"
#include "HardwareDescriptorStore.h"

namespace EnvNode {

enum class ModuleDiscoverySource : uint8_t {
    None,
    Descriptor,
    LegacyEmidV1,
};

struct ModuleDiscoveryResult {
    ModuleDiscoveryResult(
        ModuleSlot slotId = ModuleSlot::A,
        uint8_t address = 0)
        : slot(slotId), eepromAddress(address) {
    }

    ModuleSlot slot = ModuleSlot::A;
    uint8_t eepromAddress = 0;
    ModuleIdentityStatus status = ModuleIdentityStatus::StorageUnavailable;
    ModuleIdentity identity = {};
    const ModuleProfile* profile = nullptr;
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
    DescriptorTextView descriptorSerialNumber = {};

    bool identified() const;
};

class ModuleDiscoveryService {
public:
    // Legacy EMID v1 discovery for the two physical slots of the current
    // EnvNode Mini. It is diagnostic only and never activates a driver.
    static constexpr size_t SlotCount = 2;
    static constexpr uint8_t SlotAEepromAddress = 0x52;
    static constexpr uint8_t SlotBEepromAddress = 0x53;

    ModuleDiscoveryService(
        ModuleIdentityStore& slotAStore,
        ModuleIdentityStore& slotBStore);
    ModuleDiscoveryService(
        ModuleIdentityStore& slotAStore,
        ModuleIdentityStore& slotBStore,
        HardwareDescriptorStore& slotADescriptorStore,
        HardwareDescriptorStore& slotBDescriptorStore);

    void scan();
    const ModuleDiscoveryResult* result(ModuleSlot slot) const;

private:
    ModuleIdentityStore* stores_[SlotCount];
    HardwareDescriptorStore* descriptorStores_[SlotCount] = {};
    uint8_t descriptorPayloads_[SlotCount][HardwareDescriptorStore::MaximumPayloadSize] = {};
    HardwareDescriptor descriptorScratch_ = {};
    ModuleDiscoveryResult results_[SlotCount];
};

const char* moduleIdentityStatusName(ModuleIdentityStatus status);
const char* moduleDiscoverySourceName(ModuleDiscoverySource source);

} // namespace EnvNode
