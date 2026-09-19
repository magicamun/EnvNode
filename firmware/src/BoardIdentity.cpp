#include "BoardIdentity.h"

namespace EnvNode {

bool isKnownBoardProfileId(uint16_t encodedId) {
    return encodedId == static_cast<uint16_t>(BoardProfileId::EnvNodeMainboard);
}

bool decodeBoardProfileId(uint16_t encodedId, BoardProfileId& profileId) {
    if (!isKnownBoardProfileId(encodedId)) {
        return false;
    }

    profileId = BoardProfileId::EnvNodeMainboard;
    return true;
}

uint16_t encodeBoardProfileId(BoardProfileId profileId) {
    return static_cast<uint16_t>(profileId);
}

BoardIdentityStatus validateBoardIdentity(const BoardIdentity& identity) {
    if (!isKnownBoardProfileId(encodeBoardProfileId(identity.profileId))) {
        return BoardIdentityStatus::UnknownBoardProfile;
    }

    if (identity.revision.major == 0xFF && identity.revision.minor == 0xFF) {
        return BoardIdentityStatus::InvalidRevision;
    }

    switch (identity.profileId) {
        case BoardProfileId::EnvNodeMainboard:
            if (identity.revision.major != 0 || identity.revision.minor != 3) {
                return BoardIdentityStatus::UnsupportedRevision;
            }
            break;
        default:
            return BoardIdentityStatus::UnknownBoardProfile;
    }

    if (identity.serialNumber == 0) {
        return BoardIdentityStatus::UnassignedSerial;
    }

    return BoardIdentityStatus::Valid;
}

} // namespace EnvNode
