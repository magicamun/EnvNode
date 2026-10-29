#pragma once
#include "ProvisioningSnapshot.h"

#include "BoardIdentityCodec.h"
#include "IBoardIdentityStorage.h"

namespace EnvNode {

struct BoardIdentityReadResult {
    BoardIdentityStatus status = BoardIdentityStatus::StorageUnavailable;
    BoardIdentity identity = {};
};

enum class BoardIdentityWriteStatus : uint8_t {
    Success,
    InvalidIdentity,
    WriteFailed,
    ReadbackFailed,
    ReadbackInvalid,
    ReadbackMismatch,
};

class BoardIdentityStore {
public:
    explicit BoardIdentityStore(IBoardIdentityStorage& storage);

    BoardIdentityReadResult read(bool requireKnownProfile = true);
    bool snapshot(ProvisioningSnapshot&);
    BoardIdentityWriteStatus write(const BoardIdentity& identity, bool requireKnownProfile = true);

private:
    IBoardIdentityStorage& storage_;
};

} // namespace EnvNode
