#include "BoardProfile.h"
#include <initializer_list>

namespace EnvNode {
namespace {

#if defined(ENVNODE_BOARD_MAINBOARD)

#include "HardwareProfiles.generated.inc"

const BoardProfile* selectedProfile = nullptr;
bool selectionFrozen = false;

#else
#error "No EnvNode Board Profile selected. Define exactly one ENVNODE_BOARD_* build flag."
#endif

} // namespace

const BoardProfile& currentBoardProfile() {
    if (selectedProfile == nullptr) {
        selectedProfile = boardProfile(buildFallbackBoardProfileId());
    }
    selectionFrozen = true;
    return *selectedProfile;
}

const BoardProfile* boardProfile(BoardProfileId id) {
    for (size_t index = 0; index < boardProfileCount(); ++index) {
        if (BoardProfiles[index]->id == id) {
            return BoardProfiles[index];
        }
    }
    return nullptr;
}

const BoardProfile* boardProfile(BoardProfileId id, BoardRevision revision) {
    const auto* legacy=boardProfile(id);
    if(legacy && legacy->revision.major==revision.major && legacy->revision.minor==revision.minor)return legacy;
    for(const auto* p:RevisionProfiles)
        if(p->id==id && p->revision.major==revision.major && p->revision.minor==revision.minor)return p;
    return nullptr;
}

size_t boardProfileCount() {
    return sizeof(BoardProfiles) / sizeof(BoardProfiles[0]);
}

const BoardProfile* boardProfileAt(size_t index) {
    return index < boardProfileCount() ? BoardProfiles[index] : nullptr;
}

BoardProfileId buildFallbackBoardProfileId() {
#if defined(ENVNODE_BOARD_MAINBOARD)
    return BoardProfileId::EnvNodeMainboard;
#endif
}

bool selectCurrentBoardProfile(BoardProfileId id) {
    const BoardProfile* profile = boardProfile(id);
    if (profile == nullptr || selectionFrozen) {
        return false;
    }
    selectedProfile = profile;
    selectionFrozen = true;
    return true;
}

bool selectCurrentBoardProfile(BoardProfileId id, BoardRevision revision) {
    const BoardProfile* profile=boardProfile(id,revision);
    if(!profile || selectionFrozen)return false;
    selectedProfile=profile;selectionFrozen=true;return true;
}

const BoardModuleSlotCapability* boardModuleSlot(
    const BoardProfile& board,
    ModuleSlot slot) {
    for (size_t index = 0; index < board.moduleSlotCount; ++index) {
        if (board.moduleSlots[index].slot == slot) return &board.moduleSlots[index];
    }
    return nullptr;
}



} // namespace EnvNode
