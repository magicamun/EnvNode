// EnvNode: see repository LICENSE. Generated; do not edit.
const BoardGpioCapability Hardware0Gpios[] = {
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
const BoardI2CBusCapability Hardware0Buses[] = {
    {I2CBus::I2C0, GpioResource(21), GpioResource(22)},
    {I2CBus::I2C1, GpioResource(25), GpioResource(26)},
};
const BoardModuleSlotCapability Hardware0Slots[] = {
    {ModuleSlot::A, 82, GpioResource(4), GpioResource(13), GpioResource(5), true},
    {ModuleSlot::B, 83, GpioResource(14), GpioResource(16), GpioResource(27), true},
};
const BoardProfile Hardware0Profile = {BoardProfileId::EnvNodeMainboard, "EnvNode Mainboard", {0,3}, Hardware0Gpios, 15, Hardware0Buses, 2, Hardware0Slots, 2, GpioResource(18), GpioResource(19), GpioResource(23)};
const BoardGpioCapability Hardware1Gpios[] = {
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
const BoardI2CBusCapability Hardware1Buses[] = {
    {I2CBus::I2C0, GpioResource(21), GpioResource(22)},
    {I2CBus::I2C1, GpioResource(25), GpioResource(26)},
};
const BoardModuleSlotCapability Hardware1Slots[] = {
    {ModuleSlot::A, 82, GpioResource(33), GpioResource(13), GpioResource(5), true},
};
const BoardProfile Hardware1Profile = {BoardProfileId::EnvNodeWeather, "EnvNode Weather", {0,2}, Hardware1Gpios, 16, Hardware1Buses, 2, Hardware1Slots, 1, GpioResource(18), GpioResource(19), GpioResource(23)};
const BoardGpioCapability Hardware2Gpios[] = {
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
const BoardI2CBusCapability Hardware2Buses[] = {
    {I2CBus::I2C0, GpioResource(21), GpioResource(22)},
    {I2CBus::I2C1, GpioResource(25), GpioResource(26)},
};
const BoardModuleSlotCapability Hardware2Slots[] = {
    {ModuleSlot::A, 82, GpioResource(33), GpioResource(13), GpioResource(5), true},
    {ModuleSlot::B, 83, GpioResource(32), GpioResource(14), GpioResource(27), true},
};
const BoardProfile Hardware2Profile = {BoardProfileId::EnvNodeMini, "EnvNode Mini", {0,8}, Hardware2Gpios, 17, Hardware2Buses, 2, Hardware2Slots, 2, GpioResource(18), GpioResource(19), GpioResource(23)};
const BoardProfile Hardware3Profile = {BoardProfileId::EnvNodeWeather, "EnvNode Weather", {0,3}, Hardware1Gpios, 16, Hardware1Buses, 2, Hardware1Slots, 1, GpioResource(18), GpioResource(19), GpioResource(23)};
const BoardProfile Hardware4Profile = {BoardProfileId::EnvNodeMini, "EnvNode Mini", {0,9}, Hardware2Gpios, 17, Hardware2Buses, 2, Hardware2Slots, 2, GpioResource(18), GpioResource(19), GpioResource(23)};
const BoardProfile* const BoardProfiles[] = {&Hardware0Profile,&Hardware1Profile,&Hardware2Profile};
const BoardProfile* const RevisionProfiles[] = {&Hardware0Profile,&Hardware1Profile,&Hardware2Profile,&Hardware3Profile,&Hardware4Profile};
