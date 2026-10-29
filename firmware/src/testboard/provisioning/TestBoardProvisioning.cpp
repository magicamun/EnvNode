// EnvNode: see repository LICENSE.
#include "TestBoardProvisioning.h"
#include "BoardIdentityProfiles.h"
#include "ModuleDescriptorWriter.h"
#include <memory>
#include <new>
namespace EnvNode {
TestBoardProvisioning::TestBoardProvisioning(IProvisioningConnection& connection,
    BoardIdentityStore& store, std::function<bool()> available,
    HardwareDescriptorStore* slotA, HardwareDescriptorStore* slotB, BoardPortMapStore* maps)
    : connection_(connection), store_(store), service_(store,maps), available_(available),
      moduleStores_{slotA, slotB} {}
BoardIdentityReadResult TestBoardProvisioning::refresh() {
    current_ = {}; // Never retain another DUT's identity after a failed read.
    if (!available()) return current_;
    if (connection_.begin()) current_ = store_.read();
    connection_.end();
    return current_;
}
bool TestBoardProvisioning::hasProvisioningBoardIdentity() const {
    return current_.status==BoardIdentityStatus::Valid || current_.status==BoardIdentityStatus::UnassignedSerial;
}
BoardIdentity TestBoardProvisioning::provisioningBoardIdentity() const {
    if (current_.status == BoardIdentityStatus::Valid || current_.status == BoardIdentityStatus::UnassignedSerial)
        return current_.identity;
    return {BoardProfileId::EnvNodeMini, {0, 8}, {}};
}
size_t TestBoardProvisioning::provisioningProfileCount() const { return boardIdentityProfileCount(); }
bool TestBoardProvisioning::provisioningProfileAt(size_t index, ProvisioningBoardProfile& profile) const {
    const auto* p=boardIdentityProfileAt(index);
    if(!p)return false;
    profile={p->id,p->displayName,p->revision,p->minimumRuntimeFirmwareVersion,p->imageUrl};return true;
}
bool TestBoardProvisioning::provisioningProfile(BoardProfileId id, ProvisioningBoardProfile& profile) const {
    const auto* p=defaultBoardIdentityProfile(id);
    return p && provisioningProfile(id,p->revision,profile);
}
bool TestBoardProvisioning::provisioningProfile(BoardProfileId id, BoardRevision revision, ProvisioningBoardProfile& profile) const {
    const auto* p=boardIdentityProfile(id,revision);
    if(!p)return false;
    profile={p->id,p->displayName,p->revision,p->minimumRuntimeFirmwareVersion,p->imageUrl};return true;
}
BoardProvisioningResult TestBoardProvisioning::provisionBoardIdentity(const BoardIdentity& identity, bool confirmed) {
    BoardProvisioningResult result;
    result.identity = identity;
    if (!confirmed) { result.status = BoardProvisioningStatus::ConfirmationRequired; return result; }
    ProvisioningBoardProfile profile;
    const auto validation = validateBoardIdentity(identity);
    if (!provisioningProfile(identity.profileId, identity.revision, profile)
        || identity.revision.major != profile.revision.major || identity.revision.minor != profile.revision.minor
        || (validation != BoardIdentityStatus::Valid && validation != BoardIdentityStatus::UnassignedSerial))
        return result;
    if (!available()) { result.status = BoardProvisioningStatus::WriteFailed; return result; }
    current_ = {};
    if (connection_.begin()) result = service_.provision(identity, true);
    else result.status = BoardProvisioningStatus::WriteFailed;
    connection_.end();
    result.rebootRequired = false; // The EEPROM belongs to an external target.
    if (result.status == BoardProvisioningStatus::Success) {
        current_.identity = identity;
        current_.status = validateBoardIdentity(identity);
    }
    return result;
}
ModuleDescriptorProvisioningResult TestBoardProvisioning::provisionModuleDescriptor(ModuleSlot slot,
    const uint8_t* payload, size_t size, bool confirmed) {
    ModuleDescriptorProvisioningResult result;
    result.slot = slot;
    if (slot != ModuleSlot::A && slot != ModuleSlot::B) {
        result.status = ModuleDescriptorProvisioningStatus::InvalidSlot; return result;
    }
    if (!confirmed) {
        result.status = ModuleDescriptorProvisioningStatus::ConfirmationRequired; return result;
    }
    if (!available() || moduleStores_[static_cast<size_t>(slot)] == nullptr) {
        result.status = ModuleDescriptorProvisioningStatus::StorageUnavailable; return result;
    }
    std::unique_ptr<HardwareDescriptor> scratch(new (std::nothrow) HardwareDescriptor);
    if (!scratch) { result.status = ModuleDescriptorProvisioningStatus::StorageUnavailable; return result; }
    result = validateModuleProvisioningPayload(slot, payload, size, true, *scratch);
    if (result.status != ModuleDescriptorProvisioningStatus::Success) return result;
    if (connection_.begin())
        result = writeModuleProvisioningPayload(*moduleStores_[static_cast<size_t>(slot)],
            slot, payload, size, true, *scratch);
    else result.status = ModuleDescriptorProvisioningStatus::StorageUnavailable;
    connection_.end();
    // The tester provisions hardware but does not operate module drivers.
    result.compatibilityStatus = HardwareDescriptorCompatibilityStatus::MissingDriver;
    return result;
}
}
