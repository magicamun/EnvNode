// EnvNode: see repository LICENSE.
#include <unity.h>
#include <cstring>
#include "TestBoardSettings.h"
#include "TestBoardRestart.h"
#include "TestBoardWeb.h"
#include "RecentLogStore.h"
#include "Update.h"
using namespace EnvNode;
UpdateClass Update;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t n) { return n; }
struct Logger : ILogger {
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
};
struct Wifi : IWiFiService {
    void begin() override {} void loop() override {}
    bool connected() const override { return false; }
    bool inSetupAccessPointMode() const override { return true; }
    String ipAddress() const override { return "192.168.4.1"; }
    String hostname() const override { return "EnvNode-Testboard"; }
    int32_t rssi() const override { return 0; }
};
struct Time : ITimeService {
    void begin() override {} void loop() override {}
    bool synchronized() const override { return false; }
    time_t now() const override { return 0; }
    bool localCivilTime(tm&) const override { return false; }
    String iso8601Utc() const override { return {}; }
    String iso8601Local() const override { return {}; }
    String iso8601Local(time_t) const override { return {}; }
    uint32_t epoch() const override { return 0; }
};
struct RestartDriver : IDeviceRestartDriver {
    int calls = 0;
    void restart() override { ++calls; }
};
struct Fixture {
    Logger logger; TestBoardSettings settings; Wifi wifi; Time time;
    LocaleFormatter formatter{settings.base()}; RecentLogStore logs;
    RestartDriver driver; TestBoardRestart restart{logger, &driver}; OTAService ota{logger, restart};
    WebHost host{logger, wifi};
    TestBoardWeb web{host, settings, wifi, time, formatter, ota, restart, logs,
        {"0.1.0", "0.1.0+test", "1", "abc", "testboard", "now", "clean", false}};
    Fixture() { settings.load(); web.begin(); }
    WebServer& server() { return host.server(); }
};
void setUp() {
    Preferences::failBeginNamespace() = String(); Preferences::failClearNamespace() = String();
    Preferences::failStringWrites() = false; Preferences::failUIntKey() = String();
    Update = UpdateClass();
    for (const char* name : {"testboard", "testboard0", "testboard1", "test_meta", "weather", "weather_meta"}) {
        Preferences p; p.begin(name); p.clear();
    }
}
void tearDown() {}
void test_fresh_tester_does_not_load_envnode_credentials() {
    Preferences old; old.begin("weather"); old.putString("wifiSSID", "EnvNodeNetwork");
    old.putString("deviceName", "WeatherStation");
    TestBoardSettings settings; TEST_ASSERT_TRUE(settings.load());
    TEST_ASSERT_TRUE(settings.base().getNetworkConfiguration().wifiSSID.isEmpty());
    TEST_ASSERT_EQUAL_STRING("EnvNode-Testboard", settings.base().getDeviceConfiguration().name.c_str());
}
void test_tester_settings_survive_reboot_and_reset_keeps_envnode_data() {
    Preferences old; old.begin("weather"); old.putString("deviceName", "WeatherStation");
    Preferences snapshot; snapshot.begin("weather0", false, "cfg_nvs");
    const uint8_t bytes[] = {1, 2, 3, 4};
    snapshot.putBytes("cfg_snapshot", bytes, sizeof(bytes));
    TestBoardSettings settings; TEST_ASSERT_TRUE(settings.load());
    TEST_ASSERT_TRUE(settings.base().setDeviceName("Fixture"));
    TEST_ASSERT_TRUE(settings.base().setLocale(Locale::EnglishUnitedStates));
    TEST_ASSERT_TRUE(settings.base().setTimezone("GMT0BST,M3.5.0/1,M10.5.0"));
    TestBoardSettings rebooted; TEST_ASSERT_TRUE(rebooted.load());
    TEST_ASSERT_EQUAL_STRING("Fixture", rebooted.base().getDeviceConfiguration().name.c_str());
    TEST_ASSERT_EQUAL(Locale::EnglishUnitedStates, rebooted.base().getLocale());
    TEST_ASSERT_EQUAL_STRING("GMT0BST,M3.5.0/1,M10.5.0", rebooted.base().getTimeConfiguration().timezone.c_str());
    TEST_ASSERT_TRUE(rebooted.reset());
    TEST_ASSERT_EQUAL_STRING("WeatherStation", old.getString("deviceName").c_str());
    uint8_t preserved[4] = {};
    TEST_ASSERT_EQUAL(sizeof(bytes), snapshot.getBytes("cfg_snapshot", preserved, sizeof(preserved)));
    TEST_ASSERT_EQUAL_MEMORY(bytes, preserved, sizeof(bytes));
    TestBoardSettings fresh; TEST_ASSERT_TRUE(fresh.load());
    TEST_ASSERT_EQUAL_STRING("EnvNode-Testboard", fresh.base().getDeviceConfiguration().name.c_str());
}
void test_only_base_routes_are_registered_and_available_in_setup_mode() {
    Fixture f;
    for (const char* route : {"/", "/network", "/time", "/device", "/logs", "/logs/data", "/firmware", "/style.css"}) {
        TEST_ASSERT_TRUE(f.server().request(route, HTTP_GET));
        TEST_ASSERT_EQUAL(200, f.server().responseStatus);
    }
    for (const char* route : {"/sensors", "/actuators", "/controllers", "/values", "/mqtt", "/display", "/configuration", "/hardware"})
        TEST_ASSERT_FALSE(f.server().request(route, HTTP_GET));
    f.server().request("/", HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "EnvNode Testboard · v0.1.0"));
    TEST_ASSERT_NULL(strstr(f.server().responseBody.c_str(), "href='/sensors'"));
}
void test_network_save_persists_and_requires_only_explicit_restart() {
    Fixture f;
    f.server().arguments = {{"hostname", "fixture"}, {"wifiSSID", "FixtureNetwork"},
        {"addressMode", "dhcp"}, {"changeWifiPassword", "1"}, {"wifiPassword", "password123"}};
    TEST_ASSERT_TRUE(f.server().request("/network/save", HTTP_POST));
    TEST_ASSERT_EQUAL(200, f.server().responseStatus);
    TEST_ASSERT_TRUE(f.restart.deviceRestartPending());
    TEST_ASSERT_EQUAL(0, f.driver.calls);
    TestBoardSettings rebooted; TEST_ASSERT_TRUE(rebooted.load());
    TEST_ASSERT_EQUAL_STRING("FixtureNetwork", rebooted.base().getNetworkConfiguration().wifiSSID.c_str());
    TEST_ASSERT_TRUE(f.server().request("/restart", HTTP_POST));
    TEST_ASSERT_EQUAL(1, f.driver.calls);
}
void test_failed_save_never_marks_restart_and_locale_is_live() {
    Fixture f; Preferences::failStringWrites() = true;
    f.server().arguments = {{"deviceName", "Fixture"}};
    f.server().request("/device/save", HTTP_POST);
    TEST_ASSERT_EQUAL(400, f.server().responseStatus);
    TEST_ASSERT_FALSE(f.restart.deviceRestartPending());
    Preferences::failStringWrites() = false;
    f.server().arguments = {{"locale", "en-US"}, {"timezone", f.settings.base().getTimeConfiguration().timezone},
        {"ntpServer1", "pool.ntp.org"}, {"ntpServer2", "time.nist.gov"}};
    f.server().request("/time/save", HTTP_POST);
    TEST_ASSERT_EQUAL(200, f.server().responseStatus);
    TEST_ASSERT_EQUAL(Locale::EnglishUnitedStates, f.web.webLocale());
    TEST_ASSERT_FALSE(f.restart.deviceRestartPending());
    f.server().request("/", HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(f.server().responseBody.c_str(), "<html lang='en'>"));
}
void test_factory_reset_is_scoped_to_tester_and_flushes_before_restart() {
    Fixture f;
    Preferences old; old.begin("weather"); old.putString("deviceName", "WeatherStation");
    f.settings.base().setDeviceName("Fixture");
    f.server().request("/factory-reset", HTTP_POST);
    TEST_ASSERT_EQUAL(200, f.server().responseStatus);
    TEST_ASSERT_EQUAL(1, f.driver.calls);
    TEST_ASSERT_EQUAL(1, f.server().clientState.flushes);
    TEST_ASSERT_EQUAL_STRING("WeatherStation", old.getString("deviceName").c_str());
}
void test_ota_stages_without_executing_restart() {
    Fixture f;
    f.server().uploadState.name = "firmware";
    f.server().uploadState.filename = "firmware.bin";
    f.server().arguments["firmwareSize"] = "4";
    f.server().uploadState.status = UPLOAD_FILE_START;
    f.server().uploadEvent("/firmware/upload");
    f.server().uploadState.currentSize = 4;
    f.server().uploadState.status = UPLOAD_FILE_WRITE;
    f.server().uploadEvent("/firmware/upload");
    f.server().uploadState.status = UPLOAD_FILE_END;
    f.server().uploadEvent("/firmware/upload");
    f.server().request("/firmware/upload", HTTP_POST);
    TEST_ASSERT_EQUAL(200, f.server().responseStatus);
    TEST_ASSERT_TRUE(f.ota.firmwareStaged());
    TEST_ASSERT_TRUE(f.restart.deviceRestartPending());
    TEST_ASSERT_EQUAL(0, f.driver.calls);
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_fresh_tester_does_not_load_envnode_credentials);
    RUN_TEST(test_tester_settings_survive_reboot_and_reset_keeps_envnode_data);
    RUN_TEST(test_only_base_routes_are_registered_and_available_in_setup_mode);
    RUN_TEST(test_network_save_persists_and_requires_only_explicit_restart);
    RUN_TEST(test_failed_save_never_marks_restart_and_locale_is_live);
    RUN_TEST(test_factory_reset_is_scoped_to_tester_and_flushes_before_restart);
    RUN_TEST(test_ota_stages_without_executing_restart);
    return UNITY_END();
}
