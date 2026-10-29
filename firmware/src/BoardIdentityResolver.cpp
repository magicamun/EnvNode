#include "BoardIdentityResolver.h"

#include "BoardProfile.h"
#include "HardwareDescriptorCompatibility.h"
#include <cstring>
#include <new>

namespace EnvNode {

BoardIdentityResolver::BoardIdentityResolver(
    BoardIdentityStore& store,
    BoardProfileId buildFallbackProfileId, BoardPortMapStore* maps)
    : store_(store)
    , buildFallbackProfileId_(buildFallbackProfileId), maps_(maps) {
}

const BoardIdentityResolution& BoardIdentityResolver::resolve() {
    if (resolved_) {
        return resolution_;
    }
    resolved_ = true;

    if (maps_) {
        portMap_.reset(new (std::nothrow) BoardPortMap);
        if (!portMap_) {
            resolution_.source = BoardIdentitySource::InvalidPortMap;
            resolution_.portMapStatus = BoardPortMapStatus::Unavailable;
            return resolution_;
        }
        const auto mapResult = maps_->read(*portMap_);
        resolution_.portMapStatus = mapResult.status;
        if (mapResult.status != BoardPortMapStatus::NotProvisioned) {
            const auto identity = store_.read(false);
            resolution_.identity = identity.identity;
            resolution_.recordStatus = identity.status;
            const auto& map = *portMap_;
            const bool matched = mapResult.status == BoardPortMapStatus::Valid
                && (identity.status == BoardIdentityStatus::Valid
                    || identity.status == BoardIdentityStatus::UnassignedSerial)
                && map.profileId == encodeBoardProfileId(identity.identity.profileId)
                && map.revision.major == identity.identity.revision.major
                && map.revision.minor == identity.identity.revision.minor
                && !strcmp(map.serial.value, identity.identity.serialNumber.value);
            resolution_.normalRuntimeAllowed = matched
                && boardPortMapRuntimeSupported(map, currentFirmwareDescriptorVersion());
            resolution_.source = resolution_.normalRuntimeAllowed
                ? BoardIdentitySource::PortMap : BoardIdentitySource::InvalidPortMap;
            return resolution_;
        }
        portMap_.reset();
    }

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
    resolution_.identity.serialNumber.value[0] = '\0';
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
        case BoardIdentitySource::PortMap: return "PortMap";
        case BoardIdentitySource::InvalidPortMap: return "InvalidPortMap";
        case BoardIdentitySource::BuildFallback: return "BuildFallback";
        case BoardIdentitySource::UnsupportedIdentity: return "UnsupportedIdentity";
        default: return "UnsupportedIdentity";
    }
}


} // namespace EnvNode
