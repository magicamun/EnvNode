#include "BoardProvisioningService.h"

namespace EnvNode {

BoardProvisioningService::BoardProvisioningService(BoardIdentityStore& store)
    : store_(store) {
}

BoardProvisioningResult BoardProvisioningService::provision(
    const BoardIdentity& identity,
    bool explicitlyConfirmed) {
    BoardProvisioningResult result;
    result.identity = identity;
    if (!explicitlyConfirmed) {
        result.status = BoardProvisioningStatus::ConfirmationRequired;
        return result;
    }

    const BoardIdentityStatus validation = validateBoardIdentity(identity);
    if (validation != BoardIdentityStatus::Valid
        && validation != BoardIdentityStatus::UnassignedSerial) {
        result.status = BoardProvisioningStatus::InvalidIdentity;
        return result;
    }

    switch (store_.write(identity)) {
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
