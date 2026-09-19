#include "ModuleIdentity.h"

namespace EnvNode {

bool isKnownModuleProfileId(uint16_t encodedId) {
    return encodedId == static_cast<uint16_t>(ModuleProfileId::DuoRelay)
        || encodedId == static_cast<uint16_t>(ModuleProfileId::AnalogHydroPressure);
}

bool decodeModuleProfileId(uint16_t encodedId, ModuleProfileId& profileId) {
    if (!isKnownModuleProfileId(encodedId)) {
        return false;
    }
    profileId = static_cast<ModuleProfileId>(encodedId);
    return true;
}

uint16_t encodeModuleProfileId(ModuleProfileId profileId) {
    return static_cast<uint16_t>(profileId);
}

ModuleIdentityStatus validateModuleIdentity(const ModuleIdentity& identity) {
    if (!isKnownModuleProfileId(encodeModuleProfileId(identity.profileId))) {
        return ModuleIdentityStatus::UnknownModuleProfile;
    }
    if (identity.revision.major == 0xFF && identity.revision.minor == 0xFF) {
        return ModuleIdentityStatus::InvalidRevision;
    }

    switch (identity.profileId) {
        case ModuleProfileId::DuoRelay:
        case ModuleProfileId::AnalogHydroPressure:
            if (identity.revision.major != 0 || identity.revision.minor != 3) {
                return ModuleIdentityStatus::UnsupportedRevision;
            }
            break;
        default:
            return ModuleIdentityStatus::UnknownModuleProfile;
    }

    return identity.serialNumber == 0
        ? ModuleIdentityStatus::UnassignedSerial
        : ModuleIdentityStatus::Valid;
}

} // namespace EnvNode
