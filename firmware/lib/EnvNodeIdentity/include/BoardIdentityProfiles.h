// EnvNode: see repository LICENSE.
#pragma once
#include "BoardIdentity.h"
namespace EnvNode {
struct BoardIdentityProfile {
    BoardProfileId id;
    const char* displayName;
    BoardRevision revision;
};
size_t boardIdentityProfileCount();
const BoardIdentityProfile* boardIdentityProfileAt(size_t index);
const BoardIdentityProfile* boardIdentityProfile(BoardProfileId id, BoardRevision revision);
// Legacy callers retain their original manufacturing revision.
const BoardIdentityProfile* defaultBoardIdentityProfile(BoardProfileId id);
}
