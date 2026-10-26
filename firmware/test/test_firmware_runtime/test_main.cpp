// EnvNode: see repository LICENSE.
#include <unity.h>
#include <string>
#include "FirmwareRuntime.h"
#include "IWiFiService.h"
#include "ITimeService.h"
#include "IWebService.h"
// The full native application source set also links the GPIO adapters.
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
using namespace EnvNode;
namespace {
std::string calls;
struct Wifi : IWiFiService {
    void begin() override { calls += "wifi.begin;"; }
    void loop() override { calls += "wifi.loop;"; }
    bool connected() const override { return false; }
    bool inSetupAccessPointMode() const override { return false; }
    String ipAddress() const override { return {}; }
    String hostname() const override { return {}; }
    int32_t rssi() const override { return 0; }
};
struct Time : ITimeService {
    void begin() override { calls += "time.begin;"; }
    void loop() override { calls += "time.loop;"; }
    bool synchronized() const override { return false; }
    time_t now() const override { return 0; }
    bool localCivilTime(tm&) const override { return false; }
    String iso8601Utc() const override { return {}; }
    String iso8601Local() const override { return {}; }
    String iso8601Local(time_t) const override { return {}; }
    uint32_t epoch() const override { return 0; }
};
struct Web : IWebService {
    void begin() override { calls += "web.begin;"; }
    void loop() override { calls += "web.loop;"; }
};
struct DeviceApplication : IFirmwareApplication {
    void initializeDevices() override { calls += "devices;"; }
    void beginIntegrations() override { calls += "integrations.begin;"; }
    void serviceMeasurements() override { calls += "measurements;"; }
    void serviceIntegrations() override { calls += "integrations.loop;"; }
};
struct Fixture {
    Wifi wifi; Time time; Web web; DeviceApplication application;
    FirmwareRuntime runtime{wifi, time, web, application};
};
}
void setUp() { calls.clear(); }
void tearDown() {}
void test_unstarted_runtime_does_not_service_devices_or_network() {
    Fixture f; f.runtime.loop();
    TEST_ASSERT_TRUE(calls.empty());
}
void test_start_preserves_device_and_administration_order_without_network_connection() {
    Fixture f; f.runtime.begin();
    TEST_ASSERT_EQUAL_STRING("wifi.begin;time.begin;devices;web.begin;integrations.begin;", calls.c_str());
    // Disconnected WiFi and unsynchronized time must not block device/web startup.
    calls.clear(); f.runtime.begin();
    TEST_ASSERT_TRUE(calls.empty());
}
void test_repeated_loops_preserve_measurement_web_and_integration_order() {
    Fixture f; f.runtime.begin(); calls.clear();
    f.runtime.loop(); f.runtime.loop();
    TEST_ASSERT_EQUAL_STRING("wifi.loop;time.loop;measurements;web.loop;integrations.loop;"
        "wifi.loop;time.loop;measurements;web.loop;integrations.loop;", calls.c_str());
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_unstarted_runtime_does_not_service_devices_or_network);
    RUN_TEST(test_start_preserves_device_and_administration_order_without_network_connection);
    RUN_TEST(test_repeated_loops_preserve_measurement_web_and_integration_order);
    return UNITY_END();
}
