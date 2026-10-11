// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include <unity.h>
#include "ConfigurationBackup.h"
#include "ConfigurationService.h"
#include <string>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

void setUp() {
    Preferences::failStringWrites() = false;
    Preferences::failByteReads() = false;
    Preferences::failUIntKey() = String();
    for (auto name : {"weather", "weather0", "weather1", "weather_meta"}) {
        Preferences p; p.begin(name); p.clear(); p.end();
    }
}
void tearDown() {}
String replace(const String& input, const char* before, const char* after) {
    std::string value(input.c_str()); auto pos = value.find(before);
    TEST_ASSERT_TRUE(pos != std::string::npos);
    value.replace(pos, strlen(before), after); return value.c_str();
}
void test_roundtrip_and_credentials() {
    ConfigurationService service; service.loadConfiguration();
    TEST_ASSERT_TRUE(service.setDeviceName("Garten \"West\""));
    TEST_ASSERT_TRUE(service.setWifiPassword("sensitive-wifi"));
    TEST_ASSERT_TRUE(service.setMqttPassword("sensitive-mqtt"));
    String backup; bool passwords; Configuration candidate;
    TEST_ASSERT_TRUE(encodeConfigurationBackup(service.getConfiguration(), false, "1.2.3", backup));
    TEST_ASSERT_TRUE(std::string(backup.c_str()).find("sensitive-") == std::string::npos);
    Configuration target = service.getConfiguration(); target.network.wifiPassword = "target-pass";
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup, target, candidate, passwords));
    TEST_ASSERT_FALSE(passwords);
    TEST_ASSERT_EQUAL_STRING("target-pass", candidate.network.wifiPassword.c_str());
    TEST_ASSERT_EQUAL_STRING("Garten \"West\"", candidate.device.name.c_str());
    TEST_ASSERT_TRUE(service.validateRestore(candidate));
    TEST_ASSERT_TRUE(encodeConfigurationBackup(service.getConfiguration(), true, "1.2.3", backup));
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup, target, candidate, passwords));
    TEST_ASSERT_TRUE(passwords);
    TEST_ASSERT_EQUAL_STRING("sensitive-wifi", candidate.network.wifiPassword.c_str());
    String result; TEST_ASSERT_TRUE(encodeConfigurationBackup(candidate, true, "1.2.3", result));
    TEST_ASSERT_EQUAL_STRING(backup.c_str(), result.c_str());
}
void test_reject_malformed_and_incompatible() {
    ConfigurationService service; service.loadConfiguration();
    String backup; Configuration candidate; bool passwords;
    TEST_ASSERT_TRUE(encodeConfigurationBackup(service.getConfiguration(), false, "", backup));
    for (const auto& input : {
            replace(backup, "\"version\": \"1\"", "\"version\": \"2\""),
            replace(backup, "\"boardProfile\": \"0\"", "\"boardProfile\": \"255\""),
            replace(backup, "\"mqtt.port\": \"1883\"", "\"mqtt.port\": \"65536\""),
            replace(backup, "\"mqtt.port\": \"1883\"", "\"mqtt.port\": \"-1\""),
            replace(backup, "\"sensor.1.enabled\": \"1\"", "\"sensor.1.enabled\": \"2\""),
            replace(backup, "\"format\":", "\"unknown\": \"value\", \"format\":"),
            replace(backup, "\"format\":", "\"version\": \"1\", \"format\":"),
            replace(backup, "WeatherStation", "bad\\u0000name"),
            replace(backup, "WeatherStation", "bad\\uD800name"),
            replace(backup, "WeatherStation", "bad\xC0\xAF"), backup + "garbage"}) {
        TEST_ASSERT_FALSE(decodeConfigurationBackup(input, service.getConfiguration(), candidate, passwords));
    }
    String reversed; // JSON member order is irrelevant.
    const std::string data(backup.c_str());
    const auto firstEnd = data.find(",\n");
    reversed = ("{" + data.substr(firstEnd + 2, data.rfind('}') - firstEnd - 2)
        + ",\n" + data.substr(2, firstEnd - 2) + "}").c_str();
    TEST_ASSERT_TRUE(decodeConfigurationBackup(reversed, service.getConfiguration(), candidate, passwords));
}
void test_atomic_restore_and_normal_edits() {
    ConfigurationService service; service.loadConfiguration();
    Configuration candidate = service.getConfiguration(); candidate.device.name = "Restored";
    candidate.network.wifiPassword = "password";
    TEST_ASSERT_TRUE(service.restoreConfiguration(candidate));
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("Restored", rebooted.getConfiguration().device.name.c_str());
    TEST_ASSERT_EQUAL_STRING("password", rebooted.getConfiguration().network.wifiPassword.c_str());
    TEST_ASSERT_TRUE(rebooted.setDeviceName("Edited"));
    TEST_ASSERT_TRUE(rebooted.setWifiPassword(""));
    ConfigurationService edited; edited.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("Edited", edited.getConfiguration().device.name.c_str());
    TEST_ASSERT_EQUAL_STRING("", edited.getConfiguration().network.wifiPassword.c_str());
    candidate.device.name = "Second bank";
    TEST_ASSERT_TRUE(edited.restoreConfiguration(candidate));
    ConfigurationService second; second.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("Second bank", second.getConfiguration().device.name.c_str());
    TEST_ASSERT_TRUE(second.resetToDefaults());
    ConfigurationService reset; reset.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("WeatherStation", reset.getConfiguration().device.name.c_str());
    for (auto name : {"weather", "weather0", "weather1"}) {
        Preferences bank; bank.begin(name);
        TEST_ASSERT_EQUAL(0, bank.getBytesLength("cfg_snapshot"));
        TEST_ASSERT_FALSE(bank.isKey("wifiPassword"));
    }
}
void test_failed_staging_and_commit_preserve_old_configuration() {
    ConfigurationService service; service.loadConfiguration(); service.setDeviceName("Previous");
    Configuration candidate = service.getConfiguration(); candidate.device.name = "New";
    for (unsigned failure = 0; failure < 3; ++failure) {
        Preferences::failStringWrites() = failure == 0;
        Preferences::failByteReads() = failure == 1;
        Preferences::failUIntKey() = failure == 2 ? "active" : "";
        TEST_ASSERT_FALSE(service.restoreConfiguration(candidate));
        Preferences::failStringWrites() = false; Preferences::failByteReads() = false; Preferences::failUIntKey() = String();
        ConfigurationService rebooted; rebooted.loadConfiguration();
        TEST_ASSERT_EQUAL_STRING("Previous", rebooted.getConfiguration().device.name.c_str());
        TEST_ASSERT_EQUAL_STRING("Previous", service.getConfiguration().device.name.c_str());
    }
    TEST_ASSERT_TRUE(service.restoreConfiguration(candidate));
    candidate.device.name = "Another"; Preferences::failUIntKey() = "active";
    TEST_ASSERT_FALSE(service.restoreConfiguration(candidate)); Preferences::failUIntKey() = String();
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("New", rebooted.getConfiguration().device.name.c_str());
}
void test_hardware_conflicts_rejected_without_changes() {
    ConfigurationService service; service.loadConfiguration();
    Configuration candidate = service.getConfiguration();
    candidate.sensorSlots[4] = candidate.sensorSlots[3]; candidate.sensorSlots[4].slotId = 5;
    TEST_ASSERT_FALSE(service.validateRestore(candidate));
    TEST_ASSERT_FALSE(service.restoreConfiguration(candidate));
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("WeatherStation", rebooted.getConfiguration().device.name.c_str());
    TEST_ASSERT_FALSE(rebooted.getConfiguration().sensorSlots[4].enabled);
}
void test_compact_snapshot_preserves_full_configuration() {
    ConfigurationService service; service.loadConfiguration();
    Configuration candidate = service.getConfiguration();
    // Preserve inactive implementation parameters as well as currently used settings.
    candidate.sensorSlots[8].implementationConfiguration.rainGauge.millimetersPerTip = 0.123456789F;
    candidate.sensorSlots[8].implementationConfiguration.sht4x.i2c.address = 0x44;
    std::vector<uint8_t> record;
    TEST_ASSERT_TRUE(encodeConfigurationRecord(candidate, record));
    TEST_ASSERT_LESS_THAN(8000, record.size());
    Configuration restored;
    TEST_ASSERT_TRUE(decodeConfigurationRecord(record, service.getConfiguration(), restored));
    String before, after;
    TEST_ASSERT_TRUE(encodeConfigurationBackup(candidate, true, "", before));
    TEST_ASSERT_TRUE(encodeConfigurationBackup(restored, true, "", after));
    TEST_ASSERT_EQUAL_STRING(before.c_str(), after.c_str());
    record.pop_back(); TEST_ASSERT_FALSE(decodeConfigurationRecord(record, candidate, restored));
}
void test_restore_validates_selector_against_imported_values() {
    ConfigurationService service; service.loadConfiguration();
    Configuration candidate = service.getConfiguration();
    auto& actuator = candidate.actuatorSlots[0];
    actuator.enabled = true; actuator.implementation = ActuatorImplementation::GpioOnOff;
    actuator.hardware = HardwareResourceAssignment::gpioResource(GpioResource(16));
    auto& source = candidate.controllerSlots[1];
    source.enabled = true; source.implementation = ControllerImplementation::Threshold;
    auto& threshold = source.implementationConfiguration.threshold;
    threshold.decisionOnly = true; threshold.source.sensorId = 1;
    threshold.source.measurementType = MeasurementType::Temperature;
    StoredEnumValue value; value.definition.id = 1; value.definition.name = "Mode";
    value.definition.options = {{"auto", "Automatic"}, {"on", "On"}, {"off", "Off"}};
    value.definition.defaultCode = "auto"; candidate.values.push_back(value);
    auto& selector = candidate.controllerSlots[0];
    selector.enabled = true; selector.implementation = ControllerImplementation::Selector;
    auto& mapping = selector.implementationConfiguration.selector;
    mapping.modeValueId = 1; mapping.automaticControllerId = 2; mapping.targetActuatorId = 1;
    mapping.automaticCode = "auto"; mapping.onCode = "on"; mapping.offCode = "off";
    TEST_ASSERT_TRUE(service.validateRestore(candidate));
    TEST_ASSERT_TRUE(service.restoreConfiguration(candidate));
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_EQUAL(ControllerImplementation::Selector, rebooted.getConfiguration().controllerSlots[0].implementation);
    candidate.values.clear();
    TEST_ASSERT_FALSE(service.validateRestore(candidate));
    TEST_ASSERT_FALSE(service.restoreConfiguration(candidate));
}
void test_removing_restored_module_target_survives_reboot() {
    ConfigurationService service; service.loadConfiguration();
    Configuration candidate = service.getConfiguration();
    auto& actuator = candidate.actuatorSlots[0];
    actuator.implementation = ActuatorImplementation::GpioOnOff;
    actuator.hardware = HardwareResourceAssignment::none();
    actuator.moduleTarget.moduleInstanceFingerprint[0] = 1; actuator.moduleTarget.deviceIdHash[0] = 2;
    TEST_ASSERT_TRUE(service.restoreConfiguration(candidate));
    actuator.moduleTarget = ModuleActuatorReference{};
    actuator.hardware = HardwareResourceAssignment::gpioResource(GpioResource(16));
    TEST_ASSERT_TRUE(service.setActuatorSlotConfiguration(actuator));
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_FALSE(validModuleActuatorReference(rebooted.getConfiguration().actuatorSlots[0].moduleTarget));
    TEST_ASSERT_EQUAL(16, rebooted.getConfiguration().actuatorSlots[0].hardware.gpio.number);
}
void test_hardware_reassignment_after_restore_loads_complete_candidate() {
    ConfigurationService service; service.loadConfiguration();
    Configuration candidate = service.getConfiguration();
    auto& sensor = candidate.sensorSlots[3];
    sensor.hardware = HardwareResourceAssignment::gpioResource(GpioResource(18));
    sensor.implementationConfiguration.am2302.gpio = GpioResource(18);
    auto& actuator = candidate.actuatorSlots[0];
    actuator.enabled = true; actuator.implementation = ActuatorImplementation::GpioOnOff;
    actuator.hardware = HardwareResourceAssignment::gpioResource(GpioResource(16));
    TEST_ASSERT_TRUE(service.restoreConfiguration(candidate));
    actuator.hardware = HardwareResourceAssignment::gpioResource(GpioResource(17));
    TEST_ASSERT_TRUE(service.setActuatorSlotConfiguration(actuator));
    sensor.hardware = HardwareResourceAssignment::gpioResource(GpioResource(16));
    sensor.implementationConfiguration.am2302.gpio = GpioResource(16);
    TEST_ASSERT_TRUE(service.setSensorSlotConfiguration(sensor));
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_EQUAL(16, rebooted.getConfiguration().sensorSlots[3].hardware.gpio.number);
    TEST_ASSERT_EQUAL(17, rebooted.getConfiguration().actuatorSlots[0].hardware.gpio.number);
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_roundtrip_and_credentials);
    RUN_TEST(test_reject_malformed_and_incompatible);
    RUN_TEST(test_atomic_restore_and_normal_edits);
    RUN_TEST(test_failed_staging_and_commit_preserve_old_configuration);
    RUN_TEST(test_hardware_conflicts_rejected_without_changes);
    RUN_TEST(test_compact_snapshot_preserves_full_configuration);
    RUN_TEST(test_restore_validates_selector_against_imported_values);
    RUN_TEST(test_removing_restored_module_target_survives_reboot);
    RUN_TEST(test_hardware_reassignment_after_restore_loads_complete_candidate);
    return UNITY_END();
}
