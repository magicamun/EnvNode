#include "BoardProfile.h"
#include <initializer_list>

namespace EnvNode {
namespace {

#if defined(ENVNODE_BOARD_MAINBOARD)

const BoardGpioCapability EnvNodeMainboardGpios[] = {
    {GpioResource(4), "GPIO4", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(13), "GPIO13", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(14), "GPIO14", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(15), "GPIO15", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(16), "GPIO16", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(17), "GPIO17", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(18), "GPIO18", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(19), "GPIO19", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(23), "GPIO23", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(32), "GPIO32", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup | GpioCapability::AnalogInput},
    {GpioResource(33), "GPIO33", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup | GpioCapability::AnalogInput},
    {GpioResource(34), "GPIO34", GpioCapability::AnalogInput},
    {GpioResource(35), "GPIO35", GpioCapability::AnalogInput},
    {GpioResource(36), "GPIO36", GpioCapability::AnalogInput},
    {GpioResource(39), "GPIO39", GpioCapability::AnalogInput},
};

const BoardI2CBusCapability EnvNodeMainboardI2CBuses[] = {
    {I2CBus::I2C0, GpioResource(21), GpioResource(22)},
    {I2CBus::I2C1, GpioResource(25), GpioResource(26)},
};

const BoardModuleSlotCapability EnvNodeMainboardModuleSlots[] = {
    {ModuleSlot::A, 0x52, GpioResource(4), GpioResource(13), GpioResource(5), true},
    {ModuleSlot::B, 0x53, GpioResource(14), GpioResource(16), GpioResource(27), true},
};

const BoardProfile EnvNodeMainboardProfile = {
    BoardProfileId::EnvNodeMainboard,
    "EnvNode Mainboard",
    {0, 3},
    EnvNodeMainboardGpios,
    sizeof(EnvNodeMainboardGpios) / sizeof(EnvNodeMainboardGpios[0]),
    EnvNodeMainboardI2CBuses,
    sizeof(EnvNodeMainboardI2CBuses) / sizeof(EnvNodeMainboardI2CBuses[0]),
    EnvNodeMainboardModuleSlots,
    sizeof(EnvNodeMainboardModuleSlots) / sizeof(EnvNodeMainboardModuleSlots[0]),
};

const BoardGpioCapability EnvNodeWeatherGpios[] = {
    {GpioResource(4), "GPIO4", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(5), "GPIO5", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(13), "GPIO13", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(14), "GPIO14", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(16), "GPIO16", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(17), "GPIO17", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(27), "GPIO27", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(18), "GPIO18", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(19), "GPIO19", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(23), "GPIO23", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(32), "GPIO32", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup | GpioCapability::AnalogInput},
    {GpioResource(33), "GPIO33", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup | GpioCapability::AnalogInput},
    {GpioResource(34), "GPIO34", GpioCapability::AnalogInput},
    {GpioResource(35), "GPIO35", GpioCapability::AnalogInput},
    {GpioResource(36), "GPIO36", GpioCapability::AnalogInput},
    {GpioResource(39), "GPIO39", GpioCapability::AnalogInput},
};

const BoardModuleSlotCapability EnvNodeWeatherModuleSlots[] = {
    {ModuleSlot::A, 0x52, GpioResource(33), GpioResource(13), GpioResource(5), true},
};

const BoardProfile EnvNodeWeatherProfile = {
    BoardProfileId::EnvNodeWeather,
    "EnvNode Weather",
    {0, 2},
    EnvNodeWeatherGpios,
    sizeof(EnvNodeWeatherGpios) / sizeof(EnvNodeWeatherGpios[0]),
    EnvNodeMainboardI2CBuses,
    sizeof(EnvNodeMainboardI2CBuses) / sizeof(EnvNodeMainboardI2CBuses[0]),
    EnvNodeWeatherModuleSlots,
    sizeof(EnvNodeWeatherModuleSlots) / sizeof(EnvNodeWeatherModuleSlots[0]),
};

const BoardGpioCapability EnvNodeMiniGpios[] = {
    {GpioResource(4), "GPIO4", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(5), "GPIO5", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(13), "GPIO13", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(14), "GPIO14", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(15), "GPIO15", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(16), "GPIO16", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(17), "GPIO17", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(18), "GPIO18", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(19), "GPIO19", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(23), "GPIO23", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(27), "GPIO27", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(32), "GPIO32", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup | GpioCapability::AnalogInput},
    {GpioResource(33), "GPIO33", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup | GpioCapability::AnalogInput},
    {GpioResource(34), "GPIO34", GpioCapability::AnalogInput},
    {GpioResource(35), "GPIO35", GpioCapability::AnalogInput},
    {GpioResource(36), "GPIO36", GpioCapability::AnalogInput},
    {GpioResource(39), "GPIO39", GpioCapability::AnalogInput},
};

const BoardModuleSlotCapability EnvNodeMiniModuleSlots[] = {
    {ModuleSlot::A, 0x52, GpioResource(33), GpioResource(13), GpioResource(5), true},
    {ModuleSlot::B, 0x53, GpioResource(32), GpioResource(14), GpioResource(27), true},
};

const BoardProfile EnvNodeMiniProfile = {
    BoardProfileId::EnvNodeMini,
    "EnvNode Mini",
    {0, 8},
    EnvNodeMiniGpios,
    sizeof(EnvNodeMiniGpios) / sizeof(EnvNodeMiniGpios[0]),
    EnvNodeMainboardI2CBuses,
    sizeof(EnvNodeMainboardI2CBuses) / sizeof(EnvNodeMainboardI2CBuses[0]),
    EnvNodeMiniModuleSlots,
    sizeof(EnvNodeMiniModuleSlots) / sizeof(EnvNodeMiniModuleSlots[0]),
};

// New revisions change reference/power protection and decoupling, not GPIO resources.
// Keep the original objects/API defaults so old callers and board identities survive.
const BoardProfile EnvNodeWeather03Profile = {
    BoardProfileId::EnvNodeWeather, "EnvNode Weather", {0,3},
    EnvNodeWeatherGpios, sizeof(EnvNodeWeatherGpios)/sizeof(EnvNodeWeatherGpios[0]),
    EnvNodeMainboardI2CBuses, sizeof(EnvNodeMainboardI2CBuses)/sizeof(EnvNodeMainboardI2CBuses[0]),
    EnvNodeWeatherModuleSlots, sizeof(EnvNodeWeatherModuleSlots)/sizeof(EnvNodeWeatherModuleSlots[0])
};
const BoardProfile EnvNodeMini09Profile = {
    BoardProfileId::EnvNodeMini, "EnvNode Mini", {0,9},
    EnvNodeMiniGpios, sizeof(EnvNodeMiniGpios)/sizeof(EnvNodeMiniGpios[0]),
    EnvNodeMainboardI2CBuses, sizeof(EnvNodeMainboardI2CBuses)/sizeof(EnvNodeMainboardI2CBuses[0]),
    EnvNodeMiniModuleSlots, sizeof(EnvNodeMiniModuleSlots)/sizeof(EnvNodeMiniModuleSlots[0])
};

const BoardProfile* const BoardProfiles[] = {
    &EnvNodeMainboardProfile,
    &EnvNodeWeatherProfile,
    &EnvNodeMiniProfile,
};

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
    for(const auto* p:{&EnvNodeWeather03Profile,&EnvNodeMini09Profile})
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
