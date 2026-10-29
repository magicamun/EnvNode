// EnvNode: see repository LICENSE.
#include "BoardIdentityProfiles.h"
namespace EnvNode {
namespace {
#include "HardwarePortMaps.generated.inc"
const BoardIdentityProfile Profiles[] = {
#include "HardwareIdentityProfiles.generated.inc"
};
}
size_t boardIdentityProfileCount() { return sizeof(Profiles)/sizeof(Profiles[0]); }
const BoardIdentityProfile* boardIdentityProfileAt(size_t index) { return index<boardIdentityProfileCount()?&Profiles[index]:nullptr; }
const BoardIdentityProfile* boardIdentityProfile(BoardProfileId id, BoardRevision revision) {
    for(const auto& p:Profiles)
        if(p.id==id && p.revision.major==revision.major && p.revision.minor==revision.minor)return &p;
    return nullptr;
}
const BoardIdentityProfile* defaultBoardIdentityProfile(BoardProfileId id) {
    for(const auto& p:Profiles)if(p.id==id)return &p;
    return nullptr;
}
}
