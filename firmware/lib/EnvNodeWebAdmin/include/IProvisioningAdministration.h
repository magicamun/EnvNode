// EnvNode: see repository LICENSE.
#pragma once
#include "BoardProvisioningService.h"
#include "HardwareDescriptor.h"
#include "ModuleDescriptorProvisioningTypes.h"
namespace EnvNode {
struct ProvisioningBoardProfile {
    BoardProfileId id;
    const char* displayName;
    BoardRevision revision;
    DescriptorSemanticVersion minimumRuntimeFirmwareVersion;
    const char* imageUrl;
};
class IProvisioningAdministration {
public:
    virtual ~IProvisioningAdministration() = default;
    virtual bool hasProvisioningBoardIdentity() const { return true; }
    virtual BoardIdentity provisioningBoardIdentity() const = 0;
    virtual const char* provisioningBoardBatch() const { return ""; }
    virtual const char* provisioningBoardDate() const { return ""; }
    virtual size_t provisioningProfileCount() const = 0;
    virtual bool provisioningProfileAt(size_t index, ProvisioningBoardProfile& profile) const = 0;
    virtual bool provisioningProfile(BoardProfileId id, ProvisioningBoardProfile& profile) const = 0;
    // Compatible extension: existing applications may implement only the ID lookup.
    virtual bool provisioningProfile(BoardProfileId id, BoardRevision revision, ProvisioningBoardProfile& profile) const {
        return provisioningProfile(id,profile) && profile.revision.major==revision.major && profile.revision.minor==revision.minor;
    }
    virtual BoardProvisioningResult provisionBoardIdentity(const BoardIdentity& identity, bool confirmed) = 0;
    virtual BoardProvisioningResult provisionBoardPortMap(const BoardPortMap&, bool) {
        return BoardProvisioningResult{};
    }
    virtual ModuleDescriptorProvisioningResult provisionModuleDescriptor(ModuleSlot slot,
        const uint8_t* payload, size_t size, bool confirmed) = 0;
};
}
