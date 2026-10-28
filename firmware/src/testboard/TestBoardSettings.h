// EnvNode: see repository LICENSE.
#pragma once
#include "BaseSettingsService.h"
namespace EnvNode {
// All settings belong to the fixture, never to a connected DUT or EnvNode model.
class TestBoardSettings {
public:
    TestBoardSettings();
    bool load();
    bool reset();
    BaseSettingsService& base() { return settings_; }
private:
    ConfigurationStorage storage_;
    DeviceConfiguration device_;
    NetworkConfiguration network_;
    TimeConfiguration time_;
    LocaleConfiguration locale_;
    BaseSettingsService settings_;
};
}
