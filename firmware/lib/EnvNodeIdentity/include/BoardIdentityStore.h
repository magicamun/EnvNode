#pragma once

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

    BoardIdentityReadResult read();
    BoardIdentityWriteStatus write(const BoardIdentity& identity);

private:
    IBoardIdentityStorage& storage_;
};

} // namespace EnvNode
