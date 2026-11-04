#include "ConfigurationTestStorage.h"
// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include <unity.h>
#include "ConfigurationBackup.h"
#include "ConfigurationService.h"
#include "PressureReductionWebView.h"
#include "BoardProfile.h"
#include <string>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

void setUp() {
    clearConfigurationTestStorage();
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
String withoutDiagnosticTail(const String& input) {
    std::string value(input.c_str());const auto tail=value.find(",\n\"sd.systemDiagnostics\"");
    TEST_ASSERT_TRUE(tail!=std::string::npos);value.erase(tail);value+="\n}\n";return value.c_str();
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
            replace(backup, "\"version\": \"7\"", "\"version\": \"99\""),
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
    std::unique_ptr<uint8_t[]> checked;
    size_t checkedSize = 0;
    TEST_ASSERT_TRUE(encodeConfigurationRecordBuffer(candidate, checked, checkedSize));
    TEST_ASSERT_EQUAL(record.size(), checkedSize);
    TEST_ASSERT_EQUAL_MEMORY(record.data(), checked.get(), checkedSize);
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

void test_geoposition_validation_persistence_and_empty_override() {
    ConfigurationService service; service.loadConfiguration();
    GeoPosition position;
    TEST_ASSERT_TRUE(parseGeoPosition("48.123456", "-180", "0", position));
    TEST_ASSERT_TRUE(position.altitude.configured); TEST_ASSERT_EQUAL_FLOAT(0, position.altitude.value);
    TEST_ASSERT_TRUE(service.setGeoPosition(position));
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_TRUE(rebooted.getConfiguration().geoPosition.altitude.configured);
    TEST_ASSERT_EQUAL_FLOAT(position.latitude.value, rebooted.getConfiguration().geoPosition.latitude.value);
    Configuration candidate = rebooted.getConfiguration();
    TEST_ASSERT_TRUE(rebooted.restoreConfiguration(candidate));
    TEST_ASSERT_TRUE(rebooted.setGeoPosition(GeoPosition{}));
    ConfigurationService cleared; cleared.loadConfiguration();
    TEST_ASSERT_FALSE(cleared.getConfiguration().geoPosition.altitude.configured);
    for (const char* text : {"nan", "inf", "1e3", "0x10", "1,2", " ", "44330", "10001", "-1001"})
        TEST_ASSERT_FALSE(parseGeoPosition("", "", text, position));
    TEST_ASSERT_FALSE(parseGeoPosition("90.01", "", "", position));
    TEST_ASSERT_FALSE(parseGeoPosition("", "180.01", "", position));
    TEST_ASSERT_TRUE(parseGeoPosition("", "", "694", position));
    Preferences::failStringWrites() = true;
    TEST_ASSERT_FALSE(cleared.setGeoPosition(position));
    TEST_ASSERT_FALSE(cleared.getConfiguration().geoPosition.altitude.configured);
    Preferences::failStringWrites() = false;
    TEST_ASSERT_TRUE(cleared.setGeoPosition(position));
    TEST_ASSERT_TRUE(cleared.resetToDefaults());
    TEST_ASSERT_FALSE(cleared.getConfiguration().geoPosition.altitude.configured);
}
void test_new_backup_roundtrip_and_legacy_json_and_snapshot_migration() {
    ConfigurationService service; service.loadConfiguration();
    Configuration current = service.getConfiguration();
    TEST_ASSERT_TRUE(parseGeoPosition("48.1", "11.2", "694", current.geoPosition));
    String backup; Configuration decoded; bool passwords;
    TEST_ASSERT_TRUE(encodeConfigurationBackup(current, true, "", backup));
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup, service.getConfiguration(), decoded, passwords));
    TEST_ASSERT_TRUE(decoded.geoPosition.altitude.configured);
    TEST_ASSERT_EQUAL_FLOAT(694, decoded.geoPosition.altitude.value);
    std::string legacy(withoutDiagnosticTail(backup).c_str());
    auto begin = legacy.find("\"geo.latitude.configured\"");
    auto end = legacy.find("\"device.name\"", begin);
    TEST_ASSERT_TRUE(begin != std::string::npos && end != std::string::npos);
    legacy.erase(begin, end - begin);
    backup = replace(String(legacy.c_str()), "\"version\": \"7\"", "\"version\": \"1\"");
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup, current, decoded, passwords));
    TEST_ASSERT_FALSE(decoded.geoPosition.altitude.configured);
    TEST_ASSERT_FALSE(decoded.geoPosition.latitude.configured);
    // Reconstruct the exact version-1 positional record: six header fields,
    // followed directly by device.name, with no location fields.
    std::vector<uint8_t> record;
    TEST_ASSERT_TRUE(encodeConfigurationRecord(current, record));
    std::vector<std::string> fields;
    for (size_t i = 0; i < record.size();) {
        const size_t length = record[i] | (record[i + 1] << 8); i += 2;
        fields.emplace_back(reinterpret_cast<const char*>(record.data() + i), length); i += length;
    }
    fields.erase(fields.end()-(7+MaxSensorSlotCount+MaxActuatorSlotCount+MaxControllerSlotCount),fields.end());
    fields[1] = "1"; fields.erase(fields.begin() + 6, fields.begin() + 17);
    record.clear();
    for (const auto& field : fields) {
        record.push_back(field.size() & 255); record.push_back(field.size() >> 8);
        record.insert(record.end(), field.begin(), field.end());
    }
    TEST_ASSERT_TRUE(decodeConfigurationRecord(record, current, decoded));
    TEST_ASSERT_FALSE(decoded.geoPosition.altitude.configured);
    TEST_ASSERT_EQUAL_STRING(current.device.name.c_str(), decoded.device.name.c_str());
    Preferences bank; bank.begin("weather");
    TEST_ASSERT_EQUAL(record.size(), bank.putBytes("cfg_snapshot", record.data(), record.size())); bank.end();
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_FALSE(rebooted.getConfiguration().geoPosition.altitude.configured);
    TEST_ASSERT_EQUAL_STRING(current.device.name.c_str(), rebooted.getConfiguration().device.name.c_str());
}

void test_pressure_temperature_configuration_validation_persistence_and_web_filter() {
    ConfigurationService service; service.loadConfiguration();
    PressureReductionConfiguration reduction; reduction.temperatureSensorId = 1; reduction.maxTemperatureAgeMs = 1234;
    TEST_ASSERT_TRUE(service.setPressureReduction(reduction));
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_EQUAL(1, rebooted.getConfiguration().pressureReduction.temperatureSensorId);
    TEST_ASSERT_EQUAL(1234, rebooted.getConfiguration().pressureReduction.maxTemperatureAgeMs);
    auto slot = rebooted.getConfiguration().sensorSlots[0]; slot.enabled = false; slot.schedule.enabled = false;
    TEST_ASSERT_FALSE(rebooted.setSensorSlotConfiguration(slot));
    reduction.temperatureSensorId = 2; TEST_ASSERT_FALSE(rebooted.setPressureReduction(reduction));
    reduction.temperatureSensorId = 16; TEST_ASSERT_FALSE(rebooted.setPressureReduction(reduction));
    reduction.temperatureSensorId = 1; reduction.maxTemperatureAgeMs = 0; TEST_ASSERT_FALSE(rebooted.setPressureReduction(reduction));
    reduction.maxTemperatureAgeMs = UINT32_MAX; TEST_ASSERT_FALSE(rebooted.setPressureReduction(reduction));
    reduction.maxTemperatureAgeMs = 60000;
    Preferences::failStringWrites() = true; TEST_ASSERT_FALSE(rebooted.setPressureReduction(reduction));
    Preferences::failStringWrites() = false;
    TEST_ASSERT_EQUAL(1234, rebooted.getConfiguration().pressureReduction.maxTemperatureAgeMs);
    auto current = rebooted.getConfiguration(); current.sensorSlots[0].name = "Outside <north>";
    String html = buildPressureReductionConfigurationHtml(current, Locale::EnglishUnitedStates);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "Outside &lt;north&gt;"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "value='1' selected"));
    TEST_ASSERT_NULL(strstr(html.c_str(), "value='2'"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "1.234"));
    TEST_ASSERT_TRUE(rebooted.restoreConfiguration(current));
    reduction.temperatureSensorId = 0; TEST_ASSERT_TRUE(rebooted.setPressureReduction(reduction));
    ConfigurationService cleared; cleared.loadConfiguration(); TEST_ASSERT_EQUAL(0, cleared.getConfiguration().pressureReduction.temperatureSensorId);
    TEST_ASSERT_TRUE(cleared.setSensorSlotConfiguration(slot));
    uint32_t age = 0;
    TEST_ASSERT_TRUE(parseTemperatureAgeSeconds("60", age)); TEST_ASSERT_EQUAL(60000, age);
    TEST_ASSERT_TRUE(parseTemperatureAgeSeconds("0.001", age)); TEST_ASSERT_EQUAL(1, age);
    TEST_ASSERT_TRUE(parseTemperatureAgeSeconds("2147483.647", age)); TEST_ASSERT_EQUAL(INT32_MAX, age);
    for (const char* text : {"", "0", "-1", "nan", "1e3", "2147483.648", "2147484", "1.0001"})
        TEST_ASSERT_FALSE(parseTemperatureAgeSeconds(text, age));
}
void test_temperature_settings_backup_v3_and_v2_migration() {
    ConfigurationService service; service.loadConfiguration(); auto current = service.getConfiguration();
    current.pressureReduction.temperatureSensorId = 1; current.pressureReduction.maxTemperatureAgeMs = 90000;
    current.geoPosition.altitude.configured = true; current.geoPosition.altitude.value = 694;
    String backup; Configuration candidate; bool passwords;
    TEST_ASSERT_TRUE(encodeConfigurationBackup(current, true, "", backup));
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup, service.getConfiguration(), candidate, passwords));
    TEST_ASSERT_EQUAL(1, candidate.pressureReduction.temperatureSensorId);
    TEST_ASSERT_EQUAL(90000, candidate.pressureReduction.maxTemperatureAgeMs);
    TEST_ASSERT_TRUE(service.restoreConfiguration(candidate));
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_EQUAL(1, rebooted.getConfiguration().pressureReduction.temperatureSensorId);
    std::string legacy(withoutDiagnosticTail(backup).c_str()); auto begin = legacy.find("\"pressure.temperatureSensorId\"");
    auto end = legacy.find("\"device.name\"", begin); legacy.erase(begin, end - begin);
    backup = replace(String(legacy.c_str()), "\"version\": \"7\"", "\"version\": \"2\"");
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup, current, candidate, passwords));
    TEST_ASSERT_EQUAL(0, candidate.pressureReduction.temperatureSensorId);
    TEST_ASSERT_TRUE(candidate.geoPosition.altitude.configured); TEST_ASSERT_EQUAL_FLOAT(694, candidate.geoPosition.altitude.value);
    std::vector<uint8_t> record; TEST_ASSERT_TRUE(encodeConfigurationRecord(current, record));
    std::vector<std::string> fields;
    for (size_t i = 0; i < record.size();) {
        size_t size = record[i] | (record[i+1] << 8); i += 2;
        fields.emplace_back(reinterpret_cast<const char*>(record.data()+i), size); i += size;
    }
    fields.erase(fields.end()-(7+MaxSensorSlotCount+MaxActuatorSlotCount+MaxControllerSlotCount),fields.end());
    fields[1] = "2"; fields.erase(fields.begin()+12, fields.begin()+17); record.clear();
    for (const auto& field : fields) {
        record.push_back(field.size() & 255); record.push_back(field.size() >> 8);
        record.insert(record.end(), field.begin(), field.end());
    }
    TEST_ASSERT_TRUE(decodeConfigurationRecord(record, current, candidate));
    TEST_ASSERT_EQUAL(0, candidate.pressureReduction.temperatureSensorId);
    TEST_ASSERT_TRUE(candidate.geoPosition.altitude.configured);
    current.sensorSlots[0].enabled = false; TEST_ASSERT_FALSE(service.validateRestore(current));
    TEST_ASSERT_TRUE(service.resetToDefaults()); TEST_ASSERT_EQUAL(0, service.getConfiguration().pressureReduction.temperatureSensorId);
}
void test_extended_diagnostic_preferences_survive_reboot_backup_and_v4_migration() {
    ConfigurationService service;service.loadConfiguration();
    auto sensor=service.getConfiguration().sensorSlots[0];sensor.extendedDiagnostics=true;TEST_ASSERT_TRUE(service.setSensorSlotConfiguration(sensor));
    auto actor=service.getConfiguration().actuatorSlots[0];actor.extendedDiagnostics=true;TEST_ASSERT_TRUE(service.setActuatorSlotConfiguration(actor));
    auto controller=service.getConfiguration().controllerSlots[0];controller.extendedDiagnostics=true;TEST_ASSERT_TRUE(service.setControllerSlotConfiguration(controller));
    auto logging=service.getConfiguration().networkLogging;logging.systemDiagnostics=true;TEST_ASSERT_TRUE(service.setNetworkLoggingConfiguration(logging));
    ConfigurationService rebooted;rebooted.loadConfiguration();const auto& c=rebooted.getConfiguration();
    TEST_ASSERT_TRUE(c.sensorSlots[0].extendedDiagnostics);TEST_ASSERT_TRUE(c.actuatorSlots[0].extendedDiagnostics);TEST_ASSERT_TRUE(c.controllerSlots[0].extendedDiagnostics);TEST_ASSERT_TRUE(c.networkLogging.systemDiagnostics);
    String backup;bool passwords;Configuration decoded;TEST_ASSERT_TRUE(encodeConfigurationBackup(c,true,"",backup));
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup,c,decoded,passwords));TEST_ASSERT_TRUE(decoded.sensorSlots[0].extendedDiagnostics && decoded.actuatorSlots[0].extendedDiagnostics && decoded.controllerSlots[0].extendedDiagnostics && decoded.networkLogging.systemDiagnostics);
    const String old=replace(withoutDiagnosticTail(backup),"\"version\": \"7\"","\"version\": \"4\"");
    TEST_ASSERT_TRUE(decodeConfigurationBackup(old,c,decoded,passwords));TEST_ASSERT_FALSE(decoded.sensorSlots[0].extendedDiagnostics || decoded.actuatorSlots[0].extendedDiagnostics || decoded.controllerSlots[0].extendedDiagnostics || decoded.networkLogging.systemDiagnostics);
    std::vector<uint8_t> record;TEST_ASSERT_TRUE(encodeConfigurationRecord(c,record));TEST_ASSERT_TRUE(decodeConfigurationRecord(record,c,decoded));TEST_ASSERT_TRUE(decoded.sensorSlots[0].extendedDiagnostics && decoded.networkLogging.systemDiagnostics);
    std::vector<std::string> fields;
    for(size_t i=0;i<record.size();){const size_t n=record[i]|(record[i+1]<<8);i+=2;fields.emplace_back(reinterpret_cast<const char*>(record.data()+i),n);i+=n;}
    fields[1]="4";fields.erase(fields.end()-(7+MaxSensorSlotCount+MaxActuatorSlotCount+MaxControllerSlotCount),fields.end());record.clear();
    for(const auto& field:fields){record.push_back(field.size()&255);record.push_back(field.size()>>8);record.insert(record.end(),field.begin(),field.end());}
    TEST_ASSERT_TRUE(decodeConfigurationRecord(record,c,decoded));TEST_ASSERT_FALSE(decoded.sensorSlots[0].extendedDiagnostics || decoded.actuatorSlots[0].extendedDiagnostics || decoded.controllerSlots[0].extendedDiagnostics || decoded.networkLogging.systemDiagnostics);
}
void test_solar_position_slot_survives_reboot_and_backup() {
    ConfigurationService service; service.loadConfiguration();
    auto slot = service.getConfiguration().sensorSlots[15];
    slot.enabled = true; slot.name = "Sun";
    slot.implementation = SensorImplementation::SolarPosition;
    slot.hardware = HardwareResourceAssignment::none();
    slot.schedule = SensorSchedule::periodic(60000);
    TEST_ASSERT_TRUE(service.setSensorSlotConfiguration(slot));
    ConfigurationService rebooted; rebooted.loadConfiguration();
    const auto& loaded = rebooted.getConfiguration().sensorSlots[15];
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorImplementation::SolarPosition),static_cast<int>(loaded.implementation));
    TEST_ASSERT_EQUAL_UINT32(60000,loaded.schedule.sampleIntervalMs);
    String backup; bool passwords; Configuration decoded;
    TEST_ASSERT_TRUE(encodeConfigurationBackup(rebooted.getConfiguration(),false,"",backup));
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup,rebooted.getConfiguration(),decoded,passwords));
    TEST_ASSERT_TRUE(service.validateRestore(decoded));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorImplementation::SolarPosition),static_cast<int>(decoded.sensorSlots[15].implementation));
    slot.hardware = HardwareResourceAssignment::gpioResource(GpioResource(16));
    TEST_ASSERT_FALSE(service.setSensorSlotConfiguration(slot));
}

void test_frequency_units_persist_and_backup_v6_migrates_v5() {
    ConfigurationService service; service.loadConfiguration();
    TEST_ASSERT_TRUE(service.setPresentationUnit(MeasurementType::RainDetectorFrequency,PresentationUnit::Kilohertz));
    ConfigurationService rebooted; rebooted.loadConfiguration();
    TEST_ASSERT_TRUE(rebooted.getConfiguration().presentation.rainDetectorFrequency==PresentationUnit::Kilohertz);
    String backup; Configuration candidate; bool passwords;
    TEST_ASSERT_TRUE(encodeConfigurationBackup(rebooted.getConfiguration(),true,"",backup));
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup,service.getConfiguration(),candidate,passwords));
    TEST_ASSERT_TRUE(candidate.presentation.rainDetectorFrequency==PresentationUnit::Kilohertz);
    std::string legacy(backup.c_str());
    const auto tail=legacy.find(",\n\"presentation.rainDetectorFrequency\"");
    TEST_ASSERT_TRUE(tail!=std::string::npos);legacy.erase(tail);legacy+="\n}\n";
    backup=replace(String(legacy.c_str()),"\"version\": \"7\"","\"version\": \"5\"");
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup,service.getConfiguration(),candidate,passwords));
    TEST_ASSERT_TRUE(candidate.presentation.rainDetectorFrequency==PresentationUnit::Hertz);
    TEST_ASSERT_TRUE(service.resetToDefaults());
    TEST_ASSERT_TRUE(service.getConfiguration().presentation.rainDetectorFrequency==PresentationUnit::Hertz);
}

void test_v5_record_defaults_frequency_without_losing_diagnostics() {
    ConfigurationService service; service.loadConfiguration();
    Configuration current=service.getConfiguration();
    current.presentation.rainDetectorFrequency=PresentationUnit::Kilohertz;
    current.networkLogging.systemDiagnostics=true;
    current.sensorSlots[0].extendedDiagnostics=true;
    std::vector<uint8_t> record;
    TEST_ASSERT_TRUE(encodeConfigurationRecord(current,record));
    std::vector<std::string> fields;
    for(size_t offset=0;offset<record.size();) {
        const size_t size=record[offset]|(record[offset+1]<<8);offset+=2;
        fields.emplace_back(reinterpret_cast<const char*>(record.data()+offset),size);offset+=size;
    }
    TEST_ASSERT_EQUAL_STRING("7",fields[1].c_str());
    fields.erase(fields.end()-5,fields.end());
    TEST_ASSERT_EQUAL_STRING("16",fields.back().c_str());
    fields[1]="5";fields.pop_back();record.clear();
    for(const auto& field:fields) {
        record.push_back(field.size()&255);record.push_back(field.size()>>8);
        record.insert(record.end(),field.begin(),field.end());
    }
    Configuration decoded;
    TEST_ASSERT_TRUE(decodeConfigurationRecord(record,current,decoded));
    TEST_ASSERT_TRUE(decoded.presentation.rainDetectorFrequency==PresentationUnit::Hertz);
    TEST_ASSERT_TRUE(decoded.networkLogging.systemDiagnostics);
    TEST_ASSERT_TRUE(decoded.sensorSlots[0].extendedDiagnostics);
}

void test_solar_and_rain_sensor_ids_coexist_in_backup_and_persistence() {
    TEST_ASSERT_EQUAL_UINT8(9,static_cast<uint8_t>(SensorImplementation::SolarPosition));
    TEST_ASSERT_EQUAL_UINT8(10,static_cast<uint8_t>(SensorImplementation::RainDetector));
    ConfigurationService service;service.loadConfiguration();
    auto solar=service.getConfiguration().sensorSlots[15];
    solar.enabled=true;solar.name="Sun";solar.implementation=SensorImplementation::SolarPosition;
    solar.hardware=HardwareResourceAssignment::none();solar.schedule=SensorSchedule::periodic(60000);
    TEST_ASSERT_TRUE(service.setSensorSlotConfiguration(solar));
    auto rain=service.getConfiguration().sensorSlots[14];
    rain.enabled=true;rain.name="Rain";rain.implementation=SensorImplementation::RainDetector;
    rain.hardware=HardwareResourceAssignment::gpioResource(GpioResource(17));
    rain.schedule=SensorSchedule::periodic(5000);
    TEST_ASSERT_TRUE(service.setSensorSlotConfiguration(rain));
    ConfigurationService rebooted;rebooted.loadConfiguration();
    TEST_ASSERT_TRUE(rebooted.getConfiguration().sensorSlots[15].implementation==SensorImplementation::SolarPosition);
    TEST_ASSERT_TRUE(rebooted.getConfiguration().sensorSlots[14].implementation==SensorImplementation::RainDetector);
    String backup;bool passwords;Configuration decoded;
    TEST_ASSERT_TRUE(encodeConfigurationBackup(rebooted.getConfiguration(),false,"",backup));
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup,service.getConfiguration(),decoded,passwords));
    TEST_ASSERT_TRUE(service.validateRestore(decoded));
    TEST_ASSERT_TRUE(decoded.sensorSlots[15].implementation==SensorImplementation::SolarPosition);
    TEST_ASSERT_TRUE(decoded.sensorSlots[14].implementation==SensorImplementation::RainDetector);
}

void test_network_logging_migration_atomic_persistence_and_backup() {
    ConfigurationService service;service.loadConfiguration();
    Preferences legacy;legacy.begin("weather");legacy.putUInt("sdStorage",12);legacy.end();
    ConfigurationService migrated;migrated.loadConfiguration();
    auto c=migrated.getConfiguration().networkLogging;
    TEST_ASSERT_FALSE(c.enabled);TEST_ASSERT_TRUE(c.systemLog&&c.systemDiagnostics);
    c.address="192.0.2.10";c.port=5514;c.enabled=true;
    TEST_ASSERT_TRUE(migrated.setNetworkLoggingConfiguration(c));
    ConfigurationService rebooted;rebooted.loadConfiguration();
    TEST_ASSERT_TRUE(rebooted.getConfiguration().networkLogging.enabled);
    TEST_ASSERT_EQUAL_STRING("192.0.2.10",rebooted.getConfiguration().networkLogging.address.c_str());
    auto invalid=c;invalid.address="host.example";TEST_ASSERT_FALSE(rebooted.setNetworkLoggingConfiguration(invalid));
    invalid=c;invalid.port=0;TEST_ASSERT_FALSE(rebooted.setNetworkLoggingConfiguration(invalid));
    Preferences::failStringWrites()=true;c.address="192.0.2.11";
    TEST_ASSERT_FALSE(rebooted.setNetworkLoggingConfiguration(c));Preferences::failStringWrites()=false;
    TEST_ASSERT_EQUAL_STRING("192.0.2.10",rebooted.getConfiguration().networkLogging.address.c_str());
    String backup;bool passwords;Configuration decoded;
    TEST_ASSERT_TRUE(encodeConfigurationBackup(rebooted.getConfiguration(),true,"",backup));
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup,rebooted.getConfiguration(),decoded,passwords));
    TEST_ASSERT_TRUE(decoded.networkLogging.enabled&&decoded.networkLogging.systemDiagnostics);
    TEST_ASSERT_EQUAL_STRING("192.0.2.10",decoded.networkLogging.address.c_str());
    TEST_ASSERT_TRUE(rebooted.restoreConfiguration(decoded));
    // A deliberately disabled/empty new configuration must not remigrate legacy bits.
    c=NetworkLoggingConfiguration{};TEST_ASSERT_TRUE(rebooted.setNetworkLoggingConfiguration(c));
    ConfigurationService cleared;cleared.loadConfiguration();TEST_ASSERT_FALSE(cleared.getConfiguration().networkLogging.systemDiagnostics);
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_network_logging_migration_atomic_persistence_and_backup);
    RUN_TEST(test_solar_position_slot_survives_reboot_and_backup);
    RUN_TEST(test_extended_diagnostic_preferences_survive_reboot_backup_and_v4_migration);
    RUN_TEST(test_frequency_units_persist_and_backup_v6_migrates_v5);
    RUN_TEST(test_v5_record_defaults_frequency_without_losing_diagnostics);
    RUN_TEST(test_solar_and_rain_sensor_ids_coexist_in_backup_and_persistence);
    RUN_TEST(test_pressure_temperature_configuration_validation_persistence_and_web_filter);
    RUN_TEST(test_temperature_settings_backup_v3_and_v2_migration);
    RUN_TEST(test_geoposition_validation_persistence_and_empty_override);
    RUN_TEST(test_new_backup_roundtrip_and_legacy_json_and_snapshot_migration);
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
