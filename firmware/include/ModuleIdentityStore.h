#pragma once

#include "IModuleIdentityStorage.h"
#include "ModuleIdentityCodec.h"

namespace EnvNode {

struct ModuleIdentityReadResult {
    ModuleIdentityStatus status = ModuleIdentityStatus::StorageUnavailable;
    ModuleIdentity identity = {};
};

enum class ModuleIdentityWriteStatus : uint8_t {
    Success,
    InvalidIdentity,
    WriteFailed,
    ReadbackFailed,
    ReadbackInvalid,
    ReadbackMismatch,
};

class ModuleIdentityStore {
public:
    explicit ModuleIdentityStore(IModuleIdentityStorage& storage);

    ModuleIdentityReadResult read();
    ModuleIdentityWriteStatus write(const ModuleIdentity& identity);

private:
    IModuleIdentityStorage& storage_;
};

} // namespace EnvNode
