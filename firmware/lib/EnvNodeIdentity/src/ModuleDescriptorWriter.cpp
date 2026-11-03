// EnvNode: see repository LICENSE.
#include "ModuleDescriptorWriter.h"
namespace EnvNode {
ModuleDescriptorProvisioningResult validateModuleProvisioningPayload(
    ModuleSlot slot, const uint8_t* payload, size_t size, bool confirmed,
    HardwareDescriptor& scratch) {
    ModuleDescriptorProvisioningResult result;
    result.slot = slot;
    if (slot != ModuleSlot::A && slot != ModuleSlot::B) {
        result.status = ModuleDescriptorProvisioningStatus::InvalidSlot;
        return result;
    }
    if (!confirmed) {
        result.status = ModuleDescriptorProvisioningStatus::ConfirmationRequired;
        return result;
    }
    if (size > HardwareDescriptorStore::MaximumPayloadSize) return result;
    result.decodeStatus = HardwareDescriptorCodec::decode(
        payload, size, HardwareDescriptorObjectKind::Module, scratch);
    bool assigned = false;
    for (uint8_t byte : scratch.instanceId) assigned = assigned || byte != 0;
    if (result.decodeStatus == HardwareDescriptorDecodeStatus::Valid && assigned)
        result.status = ModuleDescriptorProvisioningStatus::Success;
    return result;
}
ModuleDescriptorProvisioningResult writeModuleProvisioningPayload(
    HardwareDescriptorStore& store, ModuleSlot slot, const uint8_t* payload,
    size_t size, bool confirmed, HardwareDescriptor& scratch) {
    auto result = validateModuleProvisioningPayload(slot, payload, size, confirmed, scratch);
    if (result.status != ModuleDescriptorProvisioningStatus::Success) return result;
    const auto write = store.write(HardwareDescriptorObjectKind::Module, payload, size);
    result.bank = write.bank;
    result.generation = write.generation;
    switch (write.status) {
        case HardwareDescriptorStoreStatus::Valid: break;
        case HardwareDescriptorStoreStatus::StorageUnavailable:
            result.status = ModuleDescriptorProvisioningStatus::StorageUnavailable; break;
        case HardwareDescriptorStoreStatus::WriteFailed:
            result.status = ModuleDescriptorProvisioningStatus::WriteFailed; break;
        default: result.status = ModuleDescriptorProvisioningStatus::VerificationFailed; break;
    }
    return result;
}
}
