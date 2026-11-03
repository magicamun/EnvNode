#include "BoardProvisioningService.h"
#include "BoardIdentityProfiles.h"
#include <cstring>
#include <memory>
#include <new>

namespace EnvNode {

BoardProvisioningService::BoardProvisioningService(BoardIdentityStore& store, BoardPortMapStore* maps)
    : store_(store), maps_(maps) {
}

BoardProvisioningResult BoardProvisioningService::provision(
    const BoardIdentity& identity,
    bool explicitlyConfirmed, const BoardPortMap* map) {
    BoardProvisioningResult result;
    result.identity = identity;
    if (!explicitlyConfirmed) {
        result.status = BoardProvisioningStatus::ConfirmationRequired;
        return result;
    }

    const BoardIdentityStatus validation = maps_ ? validateBoardIdentityStructure(identity) : validateBoardIdentity(identity);
    if (validation != BoardIdentityStatus::Valid
        && validation != BoardIdentityStatus::UnassignedSerial) {
        result.status = BoardProvisioningStatus::InvalidIdentity;
        return result;
    }

    if (maps_) {
        std::unique_ptr<BoardPortMap> owned;
        if (!map) {
            const auto* profile = boardIdentityProfile(identity.profileId,identity.revision);
            if (!profile) { result.status = BoardProvisioningStatus::InvalidIdentity; return result; }
            owned.reset(new (std::nothrow) BoardPortMap);
            if (!owned || !BoardPortMapCodec::decode(profile->portMap,profile->portMapSize,*owned)) {
                result.status = BoardProvisioningStatus::InvalidIdentity; return result;
            }
            owned->serial=identity.serialNumber; map=owned.get();
        }
        if (!memchr(map->serial.value,0,sizeof(map->serial.value)) || map->profileId!=encodeBoardProfileId(identity.profileId)
            ||map->revision.major!=identity.revision.major||map->revision.minor!=identity.revision.minor
            ||strcmp(map->serial.value,identity.serialNumber.value)) {
            result.status = BoardProvisioningStatus::InvalidIdentity; return result;
        }
        const auto write=maps_->write(*map);
        if(write.status!=BoardPortMapStatus::Valid) {
            result.status=write.status==BoardPortMapStatus::InvalidMap ? BoardProvisioningStatus::InvalidIdentity
                : write.status==BoardPortMapStatus::VerificationFailed ? BoardProvisioningStatus::ReadbackFailed
                : BoardProvisioningStatus::WriteFailed;
            return result;
        }
    } else if (map) {result.status=BoardProvisioningStatus::InvalidIdentity;return result;}

    switch (store_.write(identity, maps_ == nullptr)) {
        case BoardIdentityWriteStatus::Success:
            result.status = BoardProvisioningStatus::Success;
            result.rebootRequired = true;
            return result;
        case BoardIdentityWriteStatus::InvalidIdentity:
            result.status = BoardProvisioningStatus::InvalidIdentity;
            return result;
        case BoardIdentityWriteStatus::WriteFailed:
            result.status = BoardProvisioningStatus::WriteFailed;
            return result;
        case BoardIdentityWriteStatus::ReadbackFailed:
            result.status = BoardProvisioningStatus::ReadbackFailed;
            return result;
        case BoardIdentityWriteStatus::ReadbackInvalid:
            result.status = BoardProvisioningStatus::ReadbackInvalid;
            return result;
        case BoardIdentityWriteStatus::ReadbackMismatch:
        default:
            result.status = BoardProvisioningStatus::ReadbackMismatch;
            return result;
    }
}

const char* boardProvisioningStatusName(BoardProvisioningStatus status) {
    switch (status) {
        case BoardProvisioningStatus::Success: return "Success";
        case BoardProvisioningStatus::ConfirmationRequired: return "ConfirmationRequired";
        case BoardProvisioningStatus::InvalidIdentity: return "InvalidIdentity";
        case BoardProvisioningStatus::WriteFailed: return "WriteFailed";
        case BoardProvisioningStatus::ReadbackFailed: return "ReadbackFailed";
        case BoardProvisioningStatus::ReadbackInvalid: return "ReadbackInvalid";
        case BoardProvisioningStatus::ReadbackMismatch: return "ReadbackMismatch";
        default: return "InvalidIdentity";
    }
}

} // namespace EnvNode
