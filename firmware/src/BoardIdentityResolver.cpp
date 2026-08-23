#include "BoardIdentityResolver.h"

#include "BoardProfile.h"

namespace EnvNode {

BoardIdentityResolver::BoardIdentityResolver(
    BoardIdentityStore& store,
    BoardProfileId buildFallbackProfileId)
    : store_(store)
    , buildFallbackProfileId_(buildFallbackProfileId) {
}

const BoardIdentityResolution& BoardIdentityResolver::resolve() {
    if (resolved_) {
        return resolution_;
    }
    resolved_ = true;

    const BoardIdentityReadResult readResult = store_.read();
    resolution_.recordStatus = readResult.status;
    resolution_.identity = readResult.identity;

    if (readResult.status == BoardIdentityStatus::Valid
        || readResult.status == BoardIdentityStatus::UnassignedSerial) {
        resolution_.source = BoardIdentitySource::EEPROM;
        resolution_.normalRuntimeAllowed = true;
        return resolution_;
    }

    if (!fallbackAllowed(readResult.status)) {
        resolution_.source = BoardIdentitySource::UnsupportedIdentity;
        return resolution_;
    }

    const BoardProfile* fallbackProfile = boardProfile(buildFallbackProfileId_);
    if (fallbackProfile == nullptr) {
        resolution_.source = BoardIdentitySource::UnsupportedIdentity;
        return resolution_;
    }

    resolution_.source = BoardIdentitySource::BuildFallback;
    resolution_.identity.profileId = fallbackProfile->id;
    resolution_.identity.revision = fallbackProfile->revision;
    resolution_.identity.serialNumber = 0;
    resolution_.normalRuntimeAllowed = true;
    return resolution_;
}

const BoardIdentityResolution& BoardIdentityResolver::resolution() const {
    return resolution_;
}

bool BoardIdentityResolver::fallbackAllowed(BoardIdentityStatus status) const {
    switch (status) {
        case BoardIdentityStatus::StorageUnavailable:
        case BoardIdentityStatus::NotProvisioned:
        case BoardIdentityStatus::InvalidMagic:
        case BoardIdentityStatus::UnsupportedFormat:
        case BoardIdentityStatus::InvalidLength:
        case BoardIdentityStatus::InvalidCRC:
            return true;
        default:
            return false;
    }
}

const char* boardIdentitySourceName(BoardIdentitySource source) {
    switch (source) {
        case BoardIdentitySource::EEPROM: return "EEPROM";
        case BoardIdentitySource::BuildFallback: return "BuildFallback";
        case BoardIdentitySource::UnsupportedIdentity: return "UnsupportedIdentity";
        default: return "UnsupportedIdentity";
    }
}

const char* boardIdentityStatusName(BoardIdentityStatus status) {
    switch (status) {
        case BoardIdentityStatus::Valid: return "Valid";
        case BoardIdentityStatus::StorageUnavailable: return "StorageUnavailable";
        case BoardIdentityStatus::NotProvisioned: return "NotProvisioned";
        case BoardIdentityStatus::InvalidMagic: return "InvalidMagic";
        case BoardIdentityStatus::UnsupportedFormat: return "UnsupportedFormat";
        case BoardIdentityStatus::InvalidLength: return "InvalidLength";
        case BoardIdentityStatus::InvalidCRC: return "InvalidCRC";
        case BoardIdentityStatus::UnknownBoardProfile: return "UnknownBoardProfile";
        case BoardIdentityStatus::InvalidRevision: return "InvalidRevision";
        case BoardIdentityStatus::UnsupportedRevision: return "UnsupportedRevision";
        case BoardIdentityStatus::UnassignedSerial: return "UnassignedSerial";
        default: return "Unknown";
    }
}

} // namespace EnvNode
