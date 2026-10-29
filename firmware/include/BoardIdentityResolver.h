#pragma once

#include "BoardIdentityStore.h"
#include "BoardPortMap.h"
#include <memory>

namespace EnvNode {

enum class BoardIdentitySource : uint8_t {
    EEPROM,
    BuildFallback,
    UnsupportedIdentity,
    PortMap,
    InvalidPortMap,
};

struct BoardIdentityResolution {
    BoardIdentitySource source = BoardIdentitySource::UnsupportedIdentity;
    BoardIdentityStatus recordStatus = BoardIdentityStatus::StorageUnavailable;
    BoardIdentity identity = {};
    char productionBatch[65] = {};
    char productionDate[11] = {};
    bool normalRuntimeAllowed = false;
    BoardPortMapStatus portMapStatus = BoardPortMapStatus::NotProvisioned;
};

class BoardIdentityResolver {
public:
    BoardIdentityResolver(
        BoardIdentityStore& store,
        BoardProfileId buildFallbackProfileId, BoardPortMapStore* maps = nullptr);

    const BoardIdentityResolution& resolve();
    const BoardIdentityResolution& resolution() const;
    const BoardPortMap* portMap() const { return portMap_.get(); }

private:
    bool fallbackAllowed(BoardIdentityStatus status) const;

    BoardIdentityStore& store_;
    BoardProfileId buildFallbackProfileId_;
    BoardIdentityResolution resolution_;
    bool resolved_ = false;
    BoardPortMapStore* maps_;
    std::unique_ptr<BoardPortMap> portMap_;
};

const char* boardIdentitySourceName(BoardIdentitySource source);

} // namespace EnvNode
