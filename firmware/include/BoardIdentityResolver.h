#pragma once

#include "BoardIdentityStore.h"

namespace EnvNode {

enum class BoardIdentitySource : uint8_t {
    EEPROM,
    BuildFallback,
    UnsupportedIdentity,
};

struct BoardIdentityResolution {
    BoardIdentitySource source = BoardIdentitySource::UnsupportedIdentity;
    BoardIdentityStatus recordStatus = BoardIdentityStatus::StorageUnavailable;
    BoardIdentity identity = {};
    bool normalRuntimeAllowed = false;
};

class BoardIdentityResolver {
public:
    BoardIdentityResolver(
        BoardIdentityStore& store,
        BoardProfileId buildFallbackProfileId);

    const BoardIdentityResolution& resolve();
    const BoardIdentityResolution& resolution() const;

private:
    bool fallbackAllowed(BoardIdentityStatus status) const;

    BoardIdentityStore& store_;
    BoardProfileId buildFallbackProfileId_;
    BoardIdentityResolution resolution_;
    bool resolved_ = false;
};

const char* boardIdentitySourceName(BoardIdentitySource source);
const char* boardIdentityStatusName(BoardIdentityStatus status);

} // namespace EnvNode
