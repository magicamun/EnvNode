// EnvNode: see repository LICENSE.
#include "TestBoardSettings.h"
namespace EnvNode {
namespace {
// Independent of weather/weather0/weather1/weather_meta and their snapshots.
constexpr ConfigurationStorageLayout Layout{"testboard", "testboard0", "testboard1", "test_meta"};
const BaseSettingsDefaults Defaults{
    "EnvNode-Testboard", "EnvNode-Testboard", "CET-1CEST,M3.5.0,M10.5.0/3",
    "pool.ntp.org", "time.nist.gov", Locale::GermanGermany};
}
TestBoardSettings::TestBoardSettings()
    : storage_(Layout), settings_(storage_, device_, network_, time_, locale_, Defaults) {}
bool TestBoardSettings::load() {
    settings_.initializeDefaults();
    if (!storage_.begin()) return false;
    settings_.loadIdentityAndNetworkOverrides();
    settings_.loadTimeAndLocaleOverrides();
    settings_.validateIdentityAndNetwork();
    settings_.validateTime();
    fixtureProfile_=storage_.preferences().getUInt("fixtureProfile",0);
    if(fixtureProfile_>3)fixtureProfile_=0;
    return true;
}
bool TestBoardSettings::saveFixtureProfile(uint32_t code) {
    if(code>3||!storage_.persistUInt("fixtureProfile",code))return false;
    fixtureProfile_=code;return true;
}
bool TestBoardSettings::reset() {
    if (!storage_.clearAll()) return false;
    fixtureProfile_=0;settings_.initializeDefaults();
    return true;
}
}
