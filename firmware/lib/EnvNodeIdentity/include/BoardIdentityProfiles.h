// EnvNode: see repository LICENSE.
#pragma once
#include "BoardIdentity.h"
#include "HardwareDescriptor.h"
namespace EnvNode {
struct BoardIdentityProfile {
    BoardProfileId id;
    const char* displayName;
    BoardRevision revision;
    DescriptorSemanticVersion minimumRuntimeFirmwareVersion;
    const char* imageUrl;
    const uint8_t* portMap;
    size_t portMapSize;
};
size_t boardIdentityProfileCount();
const BoardIdentityProfile* boardIdentityProfileAt(size_t index);
const BoardIdentityProfile* boardIdentityProfile(BoardProfileId id, BoardRevision revision);
// Legacy callers retain their original manufacturing revision.
const BoardIdentityProfile* defaultBoardIdentityProfile(BoardProfileId id);
}
