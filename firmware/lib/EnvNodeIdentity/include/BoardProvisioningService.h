#pragma once

#include "BoardIdentityStore.h"

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
    explicit BoardProvisioningService(BoardIdentityStore& store);

    BoardProvisioningResult provision(
        const BoardIdentity& identity,
        bool explicitlyConfirmed);

private:
    BoardIdentityStore& store_;
};

const char* boardProvisioningStatusName(BoardProvisioningStatus status);

} // namespace EnvNode
