#include "ModuleDiscoveryService.h"

#include <new>

namespace EnvNode {
namespace {

size_t slotIndex(ModuleSlot slot) {
    return static_cast<size_t>(slot);
}

} // namespace

bool ModuleDiscoveryResult::identified() const {
    return source == ModuleDiscoverySource::Descriptor
        && descriptorStatus == HardwareDescriptorDecodeStatus::Valid;
}

ModuleDiscoveryService::ModuleDiscoveryService(
    HardwareDescriptorStore& slotADescriptorStore,
    HardwareDescriptorStore& slotBDescriptorStore)
    : descriptorStores_{&slotADescriptorStore, &slotBDescriptorStore}
    , results_{{ModuleSlot::A, SlotAEepromAddress},
               {ModuleSlot::B, SlotBEepromAddress}} {
}

void ModuleDiscoveryService::scan() {
    for (size_t index = 0; index < SlotCount; ++index) {
        results_[index].source = ModuleDiscoverySource::None;
        results_[index].descriptorStoreStatus =
            HardwareDescriptorStoreStatus::NotProvisioned;
        results_[index].descriptorStatus =
            HardwareDescriptorDecodeStatus::InvalidCbor;
        results_[index].descriptorCompatibility =
            HardwareDescriptorCompatibilityStatus::UnsupportedPlatform;
        results_[index].descriptorTypeId = {};
        results_[index].descriptorName = {};
        results_[index].descriptorRevision = {};
        for (size_t byte = 0; byte < sizeof(results_[index].descriptorInstanceId); ++byte) {
            results_[index].descriptorInstanceId[byte] = 0;
        }
        results_[index].descriptorHasInstanceId = false;
        results_[index].descriptorSerialNumber = {};
        results_[index].descriptorProductionBatch = {};
        results_[index].descriptorProductionDate = {};
        // HardwareDescriptor is intentionally large; reset its persistent
        // storage directly instead of creating a task-stack temporary.
        results_[index].descriptor.~HardwareDescriptor();
        new (&results_[index].descriptor) HardwareDescriptor();
        const HardwareDescriptorReadResult descriptorRead =
            descriptorStores_[index]->read(
                descriptorPayloads_[index], sizeof(descriptorPayloads_[index]));
        results_[index].descriptorStoreStatus = descriptorRead.status;
        if (descriptorRead.status == HardwareDescriptorStoreStatus::Valid) {
            results_[index].source = ModuleDiscoverySource::Descriptor;
            results_[index].descriptorStatus = HardwareDescriptorCodec::decode(
                descriptorPayloads_[index], descriptorRead.envelope.payloadLength,
                HardwareDescriptorObjectKind::Module, descriptorScratch_);
            if (results_[index].descriptorStatus == HardwareDescriptorDecodeStatus::Valid) {
                results_[index].descriptorTypeId = descriptorScratch_.typeId;
                results_[index].descriptorName = descriptorScratch_.name;
                results_[index].descriptorRevision = descriptorScratch_.hardwareRevision;
                for (size_t byte = 0; byte < sizeof(results_[index].descriptorInstanceId); ++byte) {
                    results_[index].descriptorInstanceId[byte] =
                        descriptorScratch_.instanceId[byte];
                }
                results_[index].descriptorHasInstanceId = true;
                results_[index].descriptorSerialNumber = descriptorScratch_.serialNumber;
                results_[index].descriptorProductionBatch = descriptorScratch_.productionBatch;
                results_[index].descriptorProductionDate = descriptorScratch_.productionDate;
                results_[index].descriptor = descriptorScratch_;
                results_[index].descriptorCompatibility =
                    evaluateModuleDescriptorCompatibility(
                        descriptorScratch_, currentFirmwareDescriptorVersion(),
                        currentBoardProfile(), results_[index].slot).status;
            }
        } else if (descriptorRead.status != HardwareDescriptorStoreStatus::NotProvisioned
            && descriptorRead.status != HardwareDescriptorStoreStatus::StorageUnavailable) {
            results_[index].source = ModuleDiscoverySource::Descriptor;
        }
    }
}

const char* moduleDiscoverySourceName(ModuleDiscoverySource source) {
    switch (source) {
        case ModuleDiscoverySource::Descriptor: return "Descriptor";
        case ModuleDiscoverySource::None: return "None";
        default: return "None";
    }
}

const ModuleDiscoveryResult* ModuleDiscoveryService::result(ModuleSlot slot) const {
    const size_t index = slotIndex(slot);
    return index < SlotCount ? &results_[index] : nullptr;
}

} // namespace EnvNode
