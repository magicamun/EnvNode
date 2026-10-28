// EnvNode: see repository LICENSE.
#include "BoardIdentityProfiles.h"
namespace EnvNode {
namespace {
const BoardIdentityProfile Profiles[] = {
    {BoardProfileId::EnvNodeMainboard,"EnvNode Mainboard",{0,3}},
    {BoardProfileId::EnvNodeWeather,"EnvNode Weather",{0,2}},
    {BoardProfileId::EnvNodeMini,"EnvNode Mini",{0,8}},
    {BoardProfileId::EnvNodeWeather,"EnvNode Weather",{0,3}},
    {BoardProfileId::EnvNodeMini,"EnvNode Mini",{0,9}}
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
