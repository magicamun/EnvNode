#pragma once

#include "BoardIdentityStore.h"
#include "BoardPortMap.h"

namespace EnvNode {

enum class BoardProvisioningStatus : uint8_t {
    Success,
    ConfirmationRequired,
    InvalidIdentity,
    WriteFailed,
    ReadbackFailed,
    ReadbackInvalid,
    ReadbackMismatch,
};

struct BoardProvisioningResult {
    BoardProvisioningStatus status = BoardProvisioningStatus::InvalidIdentity;
    BoardIdentity identity = {};
    bool rebootRequired = false;
};

class BoardProvisioningService {
public:
    explicit BoardProvisioningService(BoardIdentityStore& store, BoardPortMapStore* maps = nullptr);

    BoardProvisioningResult provision(
        const BoardIdentity& identity,
        bool explicitlyConfirmed, const BoardPortMap* map = nullptr);

private:
    BoardIdentityStore& store_;
    BoardPortMapStore* maps_;
};

const char* boardProvisioningStatusName(BoardProvisioningStatus status);

} // namespace EnvNode
