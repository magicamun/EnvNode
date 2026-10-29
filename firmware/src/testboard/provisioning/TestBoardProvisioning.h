// EnvNode: see repository LICENSE.
#pragma once
#include <functional>
#include "IProvisioningAdministration.h"
namespace EnvNode {
// A connection is active only for one synchronous EEPROM operation.
class IProvisioningConnection {
public:
    virtual ~IProvisioningConnection() = default;
    virtual bool begin() = 0;
    virtual void end() = 0;
};
class TestBoardProvisioning : public IProvisioningAdministration {
public:
    TestBoardProvisioning(IProvisioningConnection& connection, BoardIdentityStore& store,
        std::function<bool()> available, HardwareDescriptorStore* slotA = nullptr,
        HardwareDescriptorStore* slotB = nullptr, BoardPortMapStore* maps = nullptr);
    bool available() const { return available_(); }
    BoardIdentityReadResult refresh();
    bool hasProvisioningBoardIdentity() const override;
    BoardIdentity provisioningBoardIdentity() const override;
    const char* provisioningBoardBatch() const override { return batch_; }
    const char* provisioningBoardDate() const override { return date_; }
    size_t provisioningProfileCount() const override;
    bool provisioningProfileAt(size_t index, ProvisioningBoardProfile& profile) const override;
    bool provisioningProfile(BoardProfileId id, ProvisioningBoardProfile& profile) const override;
    bool provisioningProfile(BoardProfileId id, BoardRevision revision, ProvisioningBoardProfile& profile) const override;
    BoardProvisioningResult provisionBoardPortMap(const BoardPortMap&, bool) override;
    BoardProvisioningResult provisionBoardIdentity(const BoardIdentity& identity, bool confirmed) override;
    ModuleDescriptorProvisioningResult provisionModuleDescriptor(ModuleSlot slot,
        const uint8_t*, size_t, bool) override;
private:
    IProvisioningConnection& connection_;
    BoardIdentityStore& store_;
    BoardProvisioningService service_;
    std::function<bool()> available_;
    BoardIdentityReadResult current_;
    char batch_[65] = {}, date_[11] = {};
    HardwareDescriptorStore* moduleStores_[2];
    BoardPortMapStore* maps_;
};
}
