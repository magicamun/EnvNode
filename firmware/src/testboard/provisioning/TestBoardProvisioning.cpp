// EnvNode: see repository LICENSE.
#include "TestBoardProvisioning.h"
namespace EnvNode {
namespace {
// Manufacturing metadata, independent of the running EnvNode hardware resources.
const ProvisioningBoardProfile Profiles[] = {
    {BoardProfileId::EnvNodeMini, "EnvNode Mini", {0, 8}},
    {BoardProfileId::EnvNodeWeather, "EnvNode Weather", {0, 2}},
    {BoardProfileId::EnvNodeMainboard, "EnvNode Mainboard", {0, 3}}
};
}
TestBoardProvisioning::TestBoardProvisioning(IProvisioningConnection& connection,
    BoardIdentityStore& store, std::function<bool()> available)
    : connection_(connection), store_(store), service_(store), available_(available) {}
BoardIdentityReadResult TestBoardProvisioning::refresh() {
    current_ = {}; // Never retain another DUT's identity after a failed read.
    if (!available()) return current_;
    if (connection_.begin()) current_ = store_.read();
    connection_.end();
    return current_;
}
BoardIdentity TestBoardProvisioning::provisioningBoardIdentity() const {
    if (current_.status == BoardIdentityStatus::Valid || current_.status == BoardIdentityStatus::UnassignedSerial)
        return current_.identity;
    return {BoardProfileId::EnvNodeMini, {0, 8}, {}};
}
size_t TestBoardProvisioning::provisioningProfileCount() const { return sizeof(Profiles) / sizeof(Profiles[0]); }
bool TestBoardProvisioning::provisioningProfileAt(size_t index, ProvisioningBoardProfile& profile) const {
    if (index >= provisioningProfileCount()) return false;
    profile = Profiles[index]; return true;
}
bool TestBoardProvisioning::provisioningProfile(BoardProfileId id, ProvisioningBoardProfile& profile) const {
    for (size_t i = 0; i < provisioningProfileCount(); ++i)
        if (Profiles[i].id == id) return provisioningProfileAt(i, profile);
    return false;
}
BoardProvisioningResult TestBoardProvisioning::provisionBoardIdentity(const BoardIdentity& identity, bool confirmed) {
    BoardProvisioningResult result;
    result.identity = identity;
    if (!confirmed) { result.status = BoardProvisioningStatus::ConfirmationRequired; return result; }
    ProvisioningBoardProfile profile;
    const auto validation = validateBoardIdentity(identity);
    if (!provisioningProfile(identity.profileId, profile)
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
    const uint8_t*, size_t, bool) {
    ModuleDescriptorProvisioningResult result;
    result.slot = slot; result.status = ModuleDescriptorProvisioningStatus::StorageUnavailable;
    return result;
}
}
