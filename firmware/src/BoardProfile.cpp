#include "BoardProfile.h"

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

const BoardProfile EnvNodeMainboardProfile = {
    BoardProfileId::EnvNodeMainboard,
    "EnvNode Mainboard",
    {0, 2},
    EnvNodeMainboardGpios,
    sizeof(EnvNodeMainboardGpios) / sizeof(EnvNodeMainboardGpios[0]),
    EnvNodeMainboardI2CBuses,
    sizeof(EnvNodeMainboardI2CBuses) / sizeof(EnvNodeMainboardI2CBuses[0]),
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
    switch (id) {
        case BoardProfileId::EnvNodeMainboard:
            return &EnvNodeMainboardProfile;
        default:
            return nullptr;
    }
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

} // namespace EnvNode
