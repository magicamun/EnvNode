// EnvNode: see repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <unity.h>
#include "BaseConfigurationValidation.h"
#include "ConfigurationService.h"
#include "ConfigurationBackup.h"
#include <string>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t size) { return size; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() {
    Preferences::failStringWrites() = false; Preferences::failUIntKey() = String();
    Preferences::failByteReads() = false;
    for (const auto* name : {"weather", "weather0", "weather1", "weather_meta"}) {
        Preferences p; p.begin(name); p.clear(); p.end();
    }
}
void tearDown() {}
NetworkConfiguration validStatic() {
    NetworkConfiguration n; n.hostname = "Fixture-1"; n.wifiSSID = "Lab";
    n.wifiPassword = "password"; n.addressMode = NetworkAddressMode::Static;
    n.ipv4Address = "192.168.1.20"; n.subnetMask = "255.255.255.0";
    n.gateway = "192.168.1.1"; n.dns1 = "1.1.1.1"; return n;
}
void test_network_validation_preserves_static_rules() {
    auto n = validStatic(); TEST_ASSERT_TRUE(BaseConfigurationValidation::networkConfiguration(n));
    n.gateway = "192.168.2.1"; TEST_ASSERT_FALSE(BaseConfigurationValidation::networkConfiguration(n));
    n = validStatic(); n.subnetMask = "255.0.255.0"; TEST_ASSERT_FALSE(BaseConfigurationValidation::networkConfiguration(n));
    n = validStatic(); n.ipv4Address = "0.0.0.0"; TEST_ASSERT_FALSE(BaseConfigurationValidation::networkConfiguration(n));
    n = validStatic(); n.dns2 = "invalid"; TEST_ASSERT_FALSE(BaseConfigurationValidation::networkConfiguration(n));
    n = validStatic(); n.addressMode = static_cast<NetworkAddressMode>(42);
    TEST_ASSERT_FALSE(BaseConfigurationValidation::networkConfiguration(n));
}
void test_common_validation_keeps_existing_boundaries() {
    TEST_ASSERT_FALSE(BaseConfigurationValidation::deviceName(""));
    TEST_ASSERT_TRUE(BaseConfigurationValidation::deviceName(std::string(32, 'a').c_str()));
    TEST_ASSERT_FALSE(BaseConfigurationValidation::deviceName(std::string(33, 'a').c_str()));
    TEST_ASSERT_TRUE(BaseConfigurationValidation::wifiSSID(""));
    TEST_ASSERT_TRUE(BaseConfigurationValidation::wifiSSID(std::string(32, 'a').c_str()));
    TEST_ASSERT_FALSE(BaseConfigurationValidation::wifiSSID(std::string(33, 'a').c_str()));
    TEST_ASSERT_TRUE(BaseConfigurationValidation::wifiPassword(std::string(64, 'a').c_str()));
    TEST_ASSERT_FALSE(BaseConfigurationValidation::wifiPassword(std::string(65, 'a').c_str()));
    TEST_ASSERT_FALSE(BaseConfigurationValidation::hostname("-invalid"));
    TEST_ASSERT_FALSE(BaseConfigurationValidation::hostname("invalid-"));
    TEST_ASSERT_FALSE(BaseConfigurationValidation::hostname("invalid_name"));
    TEST_ASSERT_TRUE(BaseConfigurationValidation::hostname("Fixture-1"));
    TEST_ASSERT_TRUE(BaseConfigurationValidation::timezone(std::string(128, 'a').c_str()));
    TEST_ASSERT_FALSE(BaseConfigurationValidation::timezone(std::string(129, 'a').c_str()));
    TEST_ASSERT_TRUE(BaseConfigurationValidation::ntpServer(std::string(64, 'a').c_str()));
    TEST_ASSERT_FALSE(BaseConfigurationValidation::ntpServer(std::string(65, 'a').c_str()));
}
void test_existing_configuration_exposes_same_network_by_reference() {
    ConfigurationService service; service.loadConfiguration();
    IConfigurationService& oldView = service; INetworkConfiguration& narrowView = oldView;
    TEST_ASSERT_EQUAL_PTR(&oldView.getConfiguration().network, &narrowView.getNetworkConfiguration());
    TEST_ASSERT_TRUE(service.setNetworkConfiguration(validStatic(), true));
    TEST_ASSERT_EQUAL_STRING("Fixture-1", narrowView.getNetworkConfiguration().hostname.c_str());
}
void test_legacy_nvs_keys_load_and_new_edits_survive_reboot() {
    Preferences legacy; legacy.begin("weather"); legacy.putString("deviceName", "Existing");
    legacy.putString("wifiSSID", "Legacy-Lab"); legacy.putString("wifiPassword", "legacy-password");
    legacy.putString("hostname", "Existing-Node"); legacy.end();
    ConfigurationService service; service.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("Legacy-Lab", service.getNetworkConfiguration().wifiSSID.c_str());
    TEST_ASSERT_EQUAL_STRING("legacy-password", service.getNetworkConfiguration().wifiPassword.c_str());
    TEST_ASSERT_EQUAL_STRING("Existing", service.getConfiguration().device.name.c_str());
    TEST_ASSERT_TRUE(service.setNetworkConfiguration(validStatic(), true));
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("Fixture-1", rebooted.getNetworkConfiguration().hostname.c_str());
    TEST_ASSERT_EQUAL_STRING("192.168.1.20", rebooted.getNetworkConfiguration().ipv4Address.c_str());
    TEST_ASSERT_EQUAL_STRING("password", rebooted.getNetworkConfiguration().wifiPassword.c_str());
}
void test_backup_restore_keeps_network_credentials_and_narrow_view() {
    ConfigurationService service; service.loadConfiguration();
    TEST_ASSERT_TRUE(service.setNetworkConfiguration(validStatic(), true));
    String backup; TEST_ASSERT_TRUE(encodeConfigurationBackup(service.getConfiguration(), true, "0.7.0", backup));
    Configuration candidate; bool passwords = false;
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup, service.getConfiguration(), candidate, passwords));
    TEST_ASSERT_TRUE(passwords); candidate.network.hostname = "Restored-Node";
    TEST_ASSERT_TRUE(service.restoreConfiguration(candidate));
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("Restored-Node", rebooted.getNetworkConfiguration().hostname.c_str());
    TEST_ASSERT_EQUAL_STRING("password", rebooted.getNetworkConfiguration().wifiPassword.c_str());
    TEST_ASSERT_EQUAL(static_cast<int>(NetworkAddressMode::Static), static_cast<int>(rebooted.getNetworkConfiguration().addressMode));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_network_validation_preserves_static_rules);
    RUN_TEST(test_common_validation_keeps_existing_boundaries);
    RUN_TEST(test_existing_configuration_exposes_same_network_by_reference);
    RUN_TEST(test_legacy_nvs_keys_load_and_new_edits_survive_reboot);
    RUN_TEST(test_backup_restore_keeps_network_credentials_and_narrow_view);
    return UNITY_END();
}
