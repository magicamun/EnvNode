// EnvNode: see repository LICENSE.
#pragma once
#include "BoardProvisioningService.h"
#include "ModuleDescriptorProvisioningTypes.h"
namespace EnvNode {
struct ProvisioningBoardProfile {
    BoardProfileId id;
    const char* displayName;
    BoardRevision revision;
};
class IProvisioningAdministration {
public:
    virtual ~IProvisioningAdministration() = default;
    virtual BoardIdentity provisioningBoardIdentity() const = 0;
    virtual size_t provisioningProfileCount() const = 0;
    virtual bool provisioningProfileAt(size_t index, ProvisioningBoardProfile& profile) const = 0;
    virtual bool provisioningProfile(BoardProfileId id, ProvisioningBoardProfile& profile) const = 0;
    virtual BoardProvisioningResult provisionBoardIdentity(const BoardIdentity& identity, bool confirmed) = 0;
    virtual ModuleDescriptorProvisioningResult provisionModuleDescriptor(ModuleSlot slot,
        const uint8_t* payload, size_t size, bool confirmed) = 0;
};
}
