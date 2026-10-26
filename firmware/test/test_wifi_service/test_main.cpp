// EnvNode: see repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <unity.h>
#include "WiFiService.h"
#include <string>
#include <vector>
using namespace EnvNode;
WiFiClass WiFi;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t size) { return size; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
class NetworkSettings : public INetworkConfiguration {
public:
    NetworkConfiguration value;
    NetworkSettings() { value.hostname = "Fixture"; value.wifiSSID = "Lab"; value.wifiPassword = "secret"; }
    const NetworkConfiguration& getNetworkConfiguration() const override { return value; }
};
class TestLogger : public ILogger {
public:
    std::vector<std::string> messages;
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
    void log(LogLevel, const char* text) override { messages.emplace_back(text); }
    bool contains(const char* text) const {
        for (const auto& message : messages) if (message.find(text) != std::string::npos) return true;
        return false;
    }
};
void setUp() { WiFi = WiFiClass{}; arduinoTestMillis() = 0; }
void tearDown() {}
void test_missing_credentials_start_setup_ap_once() {
    NetworkSettings settings; settings.value.wifiSSID = ""; TestLogger logger;
    WiFi.apAddress.fromString("192.168.4.1"); WiFiService service(logger, settings);
    service.begin(); service.begin(); service.loop();
    TEST_ASSERT_TRUE(service.inSetupAccessPointMode()); TEST_ASSERT_FALSE(service.connected());
    TEST_ASSERT_EQUAL(WIFI_AP, WiFi.currentMode); TEST_ASSERT_EQUAL(1, WiFi.apCalls);
    TEST_ASSERT_EQUAL(0, WiFi.beginCalls); TEST_ASSERT_EQUAL_STRING("Fixture-Setup", WiFi.apSsid.c_str());
    TEST_ASSERT_EQUAL_STRING("192.168.4.1", service.ipAddress().c_str());
    TEST_ASSERT_TRUE(service.stationAddresses().ipAddress.isEmpty());
}
void test_dhcp_connection_and_station_status() {
    NetworkSettings settings; TestLogger logger; WiFiService service(logger, settings);
    service.begin(); service.begin();
    TEST_ASSERT_EQUAL(1, WiFi.beginCalls); TEST_ASSERT_EQUAL(0, WiFi.configCalls);
    TEST_ASSERT_EQUAL(WIFI_STA, WiFi.currentMode); TEST_ASSERT_EQUAL_STRING("Lab", WiFi.connectedSsid.c_str());
    TEST_ASSERT_EQUAL_STRING("secret", WiFi.connectedPassword.c_str());
    TEST_ASSERT_EQUAL_STRING("Fixture", WiFi.configuredHostname.c_str());
    TEST_ASSERT_FALSE(service.connected());
    WiFi.address.fromString("192.168.1.20"); WiFi.mask.fromString("255.255.255.0");
    WiFi.gateway.fromString("192.168.1.1"); WiFi.firstDns.fromString("1.1.1.1");
    WiFi.secondDns.fromString("8.8.8.8"); WiFi.currentStatus = WL_CONNECTED; service.loop();
    TEST_ASSERT_TRUE(service.connected()); TEST_ASSERT_EQUAL(-42, service.rssi());
    TEST_ASSERT_EQUAL_STRING("192.168.1.20", service.stationAddresses().ipAddress.c_str());
    TEST_ASSERT_EQUAL_STRING("255.255.255.0", service.stationAddresses().subnetMask.c_str());
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", service.stationAddresses().gateway.c_str());
    TEST_ASSERT_EQUAL_STRING("1.1.1.1", service.stationAddresses().dns1.c_str());
    TEST_ASSERT_EQUAL_STRING("8.8.8.8", service.stationAddresses().dns2.c_str());
}
void configureStatic(NetworkSettings& settings) {
    settings.value.addressMode = NetworkAddressMode::Static;
    settings.value.ipv4Address = "192.168.1.20"; settings.value.gateway = "192.168.1.1";
    settings.value.subnetMask = "255.255.255.0"; settings.value.dns1 = "1.1.1.1";
}
void test_static_configuration_applies_before_connection() {
    NetworkSettings settings; configureStatic(settings); TestLogger logger; WiFiService service(logger, settings);
    service.begin(); TEST_ASSERT_EQUAL(1, WiFi.configCalls); TEST_ASSERT_EQUAL(1, WiFi.beginCalls);
    TEST_ASSERT_EQUAL_STRING("192.168.1.20", WiFi.address.toString().c_str());
    TEST_ASSERT_EQUAL_STRING("0.0.0.0", WiFi.secondDns.toString().c_str());
}
void test_invalid_static_address_falls_back_to_ap() {
    NetworkSettings settings; configureStatic(settings); settings.value.ipv4Address = "invalid";
    TestLogger logger; WiFiService service(logger, settings); service.begin();
    TEST_ASSERT_TRUE(service.inSetupAccessPointMode()); TEST_ASSERT_EQUAL(0, WiFi.beginCalls);
    TEST_ASSERT_EQUAL(0, WiFi.configCalls);
}
void test_driver_rejecting_static_configuration_falls_back_to_ap() {
    NetworkSettings settings; configureStatic(settings); WiFi.configResult = false;
    TestLogger logger; WiFiService service(logger, settings); service.begin();
    TEST_ASSERT_TRUE(service.inSetupAccessPointMode()); TEST_ASSERT_EQUAL(0, WiFi.beginCalls);
    TEST_ASSERT_EQUAL(1, WiFi.configCalls);
}
void test_timeout_scans_without_erasing_credentials_then_starts_ap() {
    NetworkSettings settings; TestLogger logger; WiFiService service(logger, settings); service.begin();
    arduinoTestMillis() = 29999; service.loop(); TEST_ASSERT_EQUAL(0, WiFi.scanCalls);
    WiFi.accessPoints.push_back({"Lab", -50, 6}); WiFi.scanResult = 1;
    arduinoTestMillis() = 30000; service.loop();
    TEST_ASSERT_TRUE(service.inSetupAccessPointMode()); TEST_ASSERT_EQUAL(1, WiFi.scanCalls);
    TEST_ASSERT_EQUAL(1, WiFi.scanDeleteCalls); TEST_ASSERT_FALSE(WiFi.lastErase);
    TEST_ASSERT_FALSE(WiFi.lastWifiOff); TEST_ASSERT_TRUE(logger.contains("target=exact-match"));
    TEST_ASSERT_FALSE(logger.contains("secret"));
}
void test_failed_scan_still_starts_setup_ap() {
    NetworkSettings settings; TestLogger logger; WiFiService service(logger, settings); service.begin();
    WiFi.scanResult = -2; arduinoTestMillis() = 30000; service.loop();
    TEST_ASSERT_TRUE(service.inSetupAccessPointMode()); TEST_ASSERT_EQUAL(0, WiFi.scanDeleteCalls);
    TEST_ASSERT_TRUE(logger.contains("scan failed"));
}
void test_disconnect_retries_after_five_seconds_using_current_settings() {
    NetworkSettings settings; TestLogger logger; WiFiService service(logger, settings); service.begin();
    WiFi.currentStatus = WL_CONNECTED; service.loop(); WiFi.currentStatus = WL_DISCONNECTED;
    arduinoTestMillis() = 100; service.loop(); TEST_ASSERT_EQUAL(1, WiFi.disconnectCalls);
    TEST_ASSERT_FALSE(service.connected()); TEST_ASSERT_EQUAL(0, service.rssi());
    settings.value.wifiSSID = "New-Lab"; settings.value.wifiPassword = "new-secret";
    arduinoTestMillis() = 5099; service.loop(); TEST_ASSERT_EQUAL(1, WiFi.beginCalls);
    arduinoTestMillis() = 5100; service.loop(); TEST_ASSERT_EQUAL(2, WiFi.beginCalls);
    TEST_ASSERT_EQUAL_STRING("New-Lab", WiFi.connectedSsid.c_str());
    TEST_ASSERT_EQUAL_STRING("new-secret", WiFi.connectedPassword.c_str());
    WiFi.currentStatus = WL_CONNECTED; service.loop(); TEST_ASSERT_TRUE(service.connected());
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_missing_credentials_start_setup_ap_once);
    RUN_TEST(test_dhcp_connection_and_station_status);
    RUN_TEST(test_static_configuration_applies_before_connection);
    RUN_TEST(test_invalid_static_address_falls_back_to_ap);
    RUN_TEST(test_driver_rejecting_static_configuration_falls_back_to_ap);
    RUN_TEST(test_timeout_scans_without_erasing_credentials_then_starts_ap);
    RUN_TEST(test_failed_scan_still_starts_setup_ap);
    RUN_TEST(test_disconnect_retries_after_five_seconds_using_current_settings);
    return UNITY_END();
}
