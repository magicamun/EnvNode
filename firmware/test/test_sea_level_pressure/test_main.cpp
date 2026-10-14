// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include <unity.h>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include "SeaLevelPressureSensor.h"
#include "SensorManager.h"
#include "SensorPropertyReader.h"
#include "MeasurementSnapshotCache.h"
#include "MeasurementPublisher.h"
#include "UnitConverter.h"
#include "DisplayPageFormatter.h"
#include "ConfigurationRuntimeEffect.h"
#include "PressureReductionWebView.h"
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
class TestConfigurationService : public IConfigurationService {
public:
    Configuration configuration;
    void loadConfiguration() override {}
    const Configuration& getConfiguration() const override { return configuration; }
    Locale getLocale() const override { return configuration.locale.locale; }
    bool setDeviceName(const String&) override { return false; }
    bool setNetworkConfiguration(const NetworkConfiguration&, bool) override { return false; }
    bool setWifiSSID(const String&) override { return false; }
    bool setWifiPassword(const String&) override { return false; }
    bool setMqttServer(const String&) override { return false; }
    bool setMqttPort(uint16_t) override { return false; }
    bool setMqttUsername(const String&) override { return false; }
    bool setMqttPassword(const String&) override { return false; }
    bool setTimezone(const String&) override { return false; }
    bool setNtpServer1(const String&) override { return false; }
    bool setNtpServer2(const String&) override { return false; }
    bool setLocale(Locale) override { return false; }
    bool setPresentationUnit(MeasurementType, PresentationUnit) override { return false; }
    bool setSensorSlotConfiguration(const SensorSlotConfiguration&) override { return false; }
    bool setActuatorSlotConfiguration(const ActuatorSlotConfiguration&) override { return false; }
    bool setControllerSlotConfiguration(const ControllerSlotConfiguration&) override { return false; }
    bool setDisplayConfiguration(const DisplayConfiguration&) override { return false; }
    bool setEnumValueDefinitions(const std::vector<EnumValueConfiguration>&) override { return false; }
    bool saveEnumValueCode(ValueId, const String&) override { return false; }
    bool resetToDefaults() override { return false; }
};

struct PublishedMessage {
    std::string topic;
    std::string payload;
    bool retained;
};

class TestMqttService : public IMqttService {
public:
    std::vector<PublishedMessage> messages;
    void begin() override {}
    void loop() override {}
    bool connected() const override { return true; }
    bool publish(const char* topic, const char* payload, bool retained) override {
        messages.push_back({topic, payload, retained});
        return true;
    }
    bool subscribe(const char*) override { return true; }
    void setMessageHandler(IMqttMessageHandler*) override {}
};

class TestTimeService : public ITimeService {
public:
    void begin() override {}
    void loop() override {}
    bool synchronized() const override { return true; }
    time_t now() const override { return 123456; }
    bool localCivilTime(tm&) const override { return false; }
    String iso8601Utc() const override { return String(); }
    String iso8601Local() const override { return String(); }
    String iso8601Local(time_t) const override { return String(); }
    uint32_t epoch() const override { return 0; }
};

class TestClock : public IMonotonicClock {
public:
    uint32_t milliseconds = 0;
    uint32_t nowMs() const override { return milliseconds; }
};

class TestMeasurementSink : public IMeasurementSink {
public:
    void emit(const Measurement&) override {}
};

class TestMeasurementObserver : public IMeasurementObserver {
public:
    void observe(const Measurement&, uint32_t) override {}
    void clear() override {}
};


class Barometer : public ISensor {
public:
    Measurement pressure;
    SensorProvenance origin = SensorProvenance::Simulated;
    Barometer() {
        pressure.type = MeasurementType::AtmosphericPressure;
        pressure.valid = true;
        pressure.value = MeasurementValue::floatingPoint(95030);
    }
    SensorId id() const override { return 3; }
    const char* type() const override { return "abstract barometer"; }
    SensorProvenance provenance() const override { return origin; }
    SensorState state() const override { return SensorState::Ready; }
    bool supports(MeasurementType type) const override { return type == MeasurementType::AtmosphericPressure; }
    void begin() override {}
    SensorOperationResult service(IMeasurementSink& output) override { output.emit(pressure); return SensorOperationResult::Completed; }
    SensorOperationResult sample(IMeasurementSink& output) override { output.emit(pressure); return SensorOperationResult::Completed; }
};
class Capture : public IMeasurementSink {
public:
    std::vector<Measurement> values;
    void emit(const Measurement& value) override { values.push_back(value); }
};
void setUp() {}
void tearDown() {}
void test_standard_formula_and_zero_height_preserve_raw() {
    Barometer sensor; GeoPosition position; SeaLevelPressureSensor composed; Capture output;
    position.altitude.configured = true; position.altitude.value = 694;
    composed.bind(sensor, position);
    composed.sample(output);
    TEST_ASSERT_EQUAL(2, output.values.size());
    float value; output.values[0].value.tryGetFloatingPoint(value);
    TEST_ASSERT_EQUAL_FLOAT(95030, value);
    TEST_ASSERT_TRUE(output.values[1].valid);
    output.values[1].value.tryGetFloatingPoint(value);
    TEST_ASSERT_FLOAT_WITHIN(15, 103250, value);
    TEST_ASSERT_EQUAL(static_cast<int>(MeasurementQuality::Estimated), static_cast<int>(output.values[1].quality));
    position.altitude.value = 0; output.values.clear(); composed.service(output);
    output.values[1].value.tryGetFloatingPoint(value); TEST_ASSERT_EQUAL_FLOAT(95030, value);
    position.altitude.value = -200; output.values.clear(); composed.sample(output);
    output.values[1].value.tryGetFloatingPoint(value); TEST_ASSERT_TRUE(value < 95030);
}
void test_missing_height_and_invalid_inputs_have_no_derived_value() {
    Barometer sensor; GeoPosition position; SeaLevelPressureSensor composed; Capture output;
    composed.bind(sensor, position); composed.sample(output);
    TEST_ASSERT_TRUE(output.values[0].valid); TEST_ASSERT_FALSE(output.values[1].valid);
    TEST_ASSERT_EQUAL(static_cast<int>(ValueKind::None), static_cast<int>(output.values[1].value.kind()));
    position.altitude.configured = true;
    for (float pressure : {0.F, -1.F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        sensor.pressure.value = MeasurementValue::floatingPoint(pressure);
        output.values.clear(); composed.sample(output); TEST_ASSERT_FALSE(output.values[1].valid);
    }
    sensor.pressure.valid = false; sensor.pressure.value = MeasurementValue::none();
    output.values.clear(); composed.sample(output); TEST_ASSERT_FALSE(output.values[1].valid);
    sensor.pressure.valid = true; sensor.pressure.value = MeasurementValue::floatingPoint(95030);
    position.altitude.value = 44330;
    output.values.clear(); composed.sample(output); TEST_ASSERT_FALSE(output.values[1].valid);
}
void test_manager_property_cache_and_mqtt_use_same_derived_measurement() {
    TestConfigurationService configuration; configuration.configuration.device.name = "WeatherStation";
    configuration.configuration.presentation.atmosphericPressure = PresentationUnit::Hectopascal;
    auto& altitude = configuration.configuration.geoPosition.altitude;
    altitude.configured = true; altitude.value = 694;
    TestTimeService time; TestClock clock; TestMqttService mqtt;
    MeasurementPublisher publisher(configuration, time, mqtt);
    MeasurementSnapshotCache cache;
    SensorManager manager(time, clock, publisher, cache);
    Barometer sensor; SeaLevelPressureSensor composed;
    composed.bind(sensor, configuration.configuration.geoPosition);
    TEST_ASSERT_EQUAL(static_cast<int>(SensorRegistrationResult::Registered),
        static_cast<int>(manager.registerSensor(composed, SensorSchedule::periodic(1000))));
    manager.begin(); manager.loop();
    const ISensor* registered = manager.sensor(3); TEST_ASSERT_NOT_NULL(registered);
    SensorPropertyReader reader(*registered, cache);
    PropertyReference reference(PropertyComponentKind::Sensor, 3, "sea_level_pressure");
    PropertyDescription description; TEST_ASSERT_TRUE(reader.describe(reference, description));
    TEST_ASSERT_EQUAL_STRING("sea_level_pressure", description.stableKey);
    PropertySnapshot snapshot;
    TEST_ASSERT_EQUAL(static_cast<int>(PropertyReadResult::Available), static_cast<int>(reader.read(reference, snapshot)));
    TEST_ASSERT_TRUE(snapshot.valid); TEST_ASSERT_EQUAL(123456, snapshot.timestamp);
    TEST_ASSERT_EQUAL(static_cast<int>(MeasurementQuality::Estimated), static_cast<int>(snapshot.quality));
    DisplayPage page;
    page.formats[0] = "NN %.0f Pa";
    page.sources[0][0] = "sensor/3/sea_level_pressure";
    const auto display = formatDisplayLine(reader, page, 0);
    TEST_ASSERT_EQUAL(static_cast<int>(DisplayLineIssue::None), static_cast<int>(display.issue));
    TEST_ASSERT_EQUAL_STRING("NN 103246 Pa", display.value.text);
    TEST_ASSERT_EQUAL(static_cast<int>(RuntimeAction::RestartSensorManager),
        static_cast<int>(runtimeActionFor(ConfigurationArea::GeoPosition)));
    bool rawFound = false, derivedFound = false;
    for (const auto& message : mqtt.messages) {
        if (message.topic.find("/atmospheric_pressure") != std::string::npos) {
            rawFound = true; TEST_ASSERT_NOT_NULL(strstr(message.payload.c_str(), "950.300"));
        }
        if (message.topic.find("/sea_level_pressure") != std::string::npos) {
            derivedFound = true;
            TEST_ASSERT_NOT_NULL(strstr(message.payload.c_str(), "\"derived\":true"));
            TEST_ASSERT_NOT_NULL(strstr(message.payload.c_str(), "standard_atmosphere"));
            TEST_ASSERT_NOT_NULL(strstr(message.payload.c_str(), "\"simulated\":true"));
            TEST_ASSERT_NOT_NULL(strstr(message.payload.c_str(), "hPa"));
        }
    }
    TEST_ASSERT_TRUE(rawFound); TEST_ASSERT_TRUE(derivedFound);
    altitude.configured = false; manager.loop();
    TEST_ASSERT_EQUAL(static_cast<int>(PropertyReadResult::Available), static_cast<int>(reader.read(reference, snapshot)));
    TEST_ASSERT_FALSE(snapshot.valid);
    float value;
    TEST_ASSERT_TRUE(UnitConverter::convert(MeasurementType::SeaLevelPressure, 100000, PresentationUnit::Kilopascal, value));
    TEST_ASSERT_EQUAL_FLOAT(100, value);
    TEST_ASSERT_TRUE(UnitConverter::convert(MeasurementType::SeaLevelPressure, 101325, PresentationUnit::InchMercury, value));
    TEST_ASSERT_FLOAT_WITHIN(.001, 29.9213, value);
}

Measurement temperature(float celsius, MeasurementQuality quality = MeasurementQuality::Good) {
    Measurement value; value.source = 1; value.timestamp = 123456;
    value.type = MeasurementType::Temperature; value.valid = true;
    value.value = MeasurementValue::floatingPoint(celsius); value.quality = quality;
    return value;
}
void test_local_temperature_formula_quality_and_provenance() {
    Barometer sensor; sensor.origin = SensorProvenance::Physical;
    GeoPosition position; position.altitude.configured = true; position.altitude.value = 694;
    PressureReductionConfiguration configuration; configuration.temperatureSensorId = 1;
    MeasurementSnapshotCache cache; TestClock clock; Capture output; SeaLevelPressureSensor composed;
    composed.bind(sensor, position, configuration, cache, clock);
    cache.observe(temperature(15.2), 0);
    composed.sample(output);
    float value; output.values.back().value.tryGetFloatingPoint(value);
    TEST_ASSERT_FLOAT_WITHIN(5, 103107, value);
    TEST_ASSERT_EQUAL(static_cast<int>(MeasurementQuality::Good), static_cast<int>(output.values.back().quality));
    PressureReductionStatus status; TEST_ASSERT_TRUE(composed.pressureReductionStatus(status));
    TEST_ASSERT_EQUAL(static_cast<int>(PressureReductionMethod::LocalTemperature), static_cast<int>(status.method));
    TEST_ASSERT_EQUAL(static_cast<int>(PressureReductionReason::None), static_cast<int>(status.reason));
    for (auto quality : {MeasurementQuality::Estimated, MeasurementQuality::Degraded, MeasurementQuality::BelowMeasurementRange}) {
        cache.observe(temperature(15.2, quality), 0); composed.sample(output);
        TEST_ASSERT_EQUAL(static_cast<int>(MeasurementQuality::Estimated), static_cast<int>(output.values.back().quality));
    }
    cache.observe(temperature(15.2), 0);
    for (auto quality : {MeasurementQuality::Degraded, MeasurementQuality::BelowMeasurementRange}) {
        sensor.pressure.quality = quality; composed.sample(output);
        TEST_ASSERT_EQUAL(static_cast<int>(quality), static_cast<int>(output.values.back().quality));
    }
    auto simulated = temperature(15.2); simulated.provenance = SensorProvenance::Simulated;
    cache.observe(simulated, 0); composed.sample(output);
    TEST_ASSERT_EQUAL(static_cast<int>(SensorProvenance::Simulated), static_cast<int>(output.values.back().provenance));
}
void test_temperature_age_boundaries_wrap_and_invalid_fallback() {
    Barometer sensor; GeoPosition position; position.altitude.configured = true; position.altitude.value = 694;
    PressureReductionConfiguration configuration; configuration.temperatureSensorId = 1; configuration.maxTemperatureAgeMs = 100;
    MeasurementSnapshotCache cache; TestClock clock; Capture output; SeaLevelPressureSensor composed;
    composed.bind(sensor, position, configuration, cache, clock);
    const auto reason = [&]() { PressureReductionStatus status; composed.pressureReductionStatus(status); return status.reason; };
    composed.sample(output);
    TEST_ASSERT_EQUAL(static_cast<int>(PressureReductionReason::TemperatureUnavailable), static_cast<int>(reason()));
    cache.observe(temperature(15.2), UINT32_MAX - 50);
    clock.milliseconds = 49; composed.sample(output);
    TEST_ASSERT_EQUAL(static_cast<int>(PressureReductionReason::None), static_cast<int>(reason()));
    clock.milliseconds = 50; composed.sample(output);
    TEST_ASSERT_EQUAL(static_cast<int>(PressureReductionReason::TemperatureStale), static_cast<int>(reason()));
    float value; output.values.back().value.tryGetFloatingPoint(value); TEST_ASSERT_FLOAT_WITHIN(5, 103246, value);
    clock.milliseconds = 0;
    for (float celsius : {-273.15F, -300.F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        cache.observe(temperature(celsius), 0); composed.sample(output);
        TEST_ASSERT_EQUAL(static_cast<int>(PressureReductionReason::TemperatureInvalid), static_cast<int>(reason()));
        TEST_ASSERT_TRUE(output.values.back().valid);
    }
    auto invalid = temperature(15.2); invalid.valid = false; invalid.value = MeasurementValue::none();
    cache.observe(invalid, 0); composed.sample(output);
    TEST_ASSERT_EQUAL(static_cast<int>(PressureReductionReason::TemperatureInvalid), static_cast<int>(reason()));
    cache.clear(); composed.sample(output);
    TEST_ASSERT_EQUAL(static_cast<int>(PressureReductionReason::TemperatureUnavailable), static_cast<int>(reason()));
}
class CombinedBarometer : public Barometer {
public:
    bool supports(MeasurementType type) const override { return type == MeasurementType::Temperature || Barometer::supports(type); }
    SensorOperationResult service(IMeasurementSink&) override { return SensorOperationResult::NoData; }
    SensorOperationResult sample(IMeasurementSink& output) override {
        auto value = temperature(15.2); value.source = InvalidSensorId; value.timestamp = 0;
        output.emit(value); return Barometer::sample(output);
    }
};
void test_same_sensor_temperature_reaches_resolver_before_pressure_and_mqtt_diagnostics() {
    TestConfigurationService configuration; configuration.configuration.device.name = "test";
    auto& position = configuration.configuration.geoPosition; position.altitude.configured = true; position.altitude.value = 694;
    auto& reduction = configuration.configuration.pressureReduction; reduction.temperatureSensorId = 3;
    TestTimeService time; TestClock clock; TestMqttService mqtt; MeasurementSnapshotCache cache;
    MeasurementPublisher publisher(configuration, time, mqtt); SensorManager manager(time, clock, publisher, cache);
    publisher.setPressureReductionStatusResolver(&manager);
    CombinedBarometer sensor; sensor.origin = SensorProvenance::Physical; SeaLevelPressureSensor composed;
    composed.bind(sensor, position, reduction, cache, clock);
    manager.registerSensor(composed, SensorSchedule::periodic(1000)); manager.begin(); manager.loop();
    TEST_ASSERT_EQUAL(3, mqtt.messages.size());
    const auto& payload = mqtt.messages.back().payload;
    TEST_ASSERT_NOT_NULL(strstr(payload.c_str(), "\"derivation\":\"local_temperature\""));
    TEST_ASSERT_NOT_NULL(strstr(payload.c_str(), "\"quality\":\"good\""));
    TEST_ASSERT_NOT_NULL(strstr(payload.c_str(), "\"temperature_source\":3"));
    TEST_ASSERT_NOT_NULL(strstr(payload.c_str(), "\"temperature_age_ms\":0"));
    TEST_ASSERT_NOT_NULL(strstr(payload.c_str(), "\"reduction_reason\":\"none\""));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_local_temperature_formula_quality_and_provenance);
    RUN_TEST(test_temperature_age_boundaries_wrap_and_invalid_fallback);
    RUN_TEST(test_same_sensor_temperature_reaches_resolver_before_pressure_and_mqtt_diagnostics);
    RUN_TEST(test_standard_formula_and_zero_height_preserve_raw);
    RUN_TEST(test_missing_height_and_invalid_inputs_have_no_derived_value);
    RUN_TEST(test_manager_property_cache_and_mqtt_use_same_derived_measurement);
    return UNITY_END();
}
