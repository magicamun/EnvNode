// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include <unity.h>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include "SeaLevelPressureSensor.h"
#include "HumidityDerivedSensor.h"
#include "SolarPositionSensor.h"
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
    bool synced = true;
    time_t utc = 1718971200; // 2024-06-21 12:00 UTC
    bool synchronized() const override { return synced; }
    time_t now() const override { return utc; }
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



class Capture : public IMeasurementSink {
public:
    std::vector<Measurement> values;
    void emit(const Measurement& value) override { values.push_back(value); }
};
class Combined : public ISensor {
public:
    bool emitTemperature = true, emitHumidity = true, reverse = false;
    bool temperatureSupported = true, humiditySupported = true, valid = true;
    MeasurementQuality temperatureQuality = MeasurementQuality::Good;
    MeasurementQuality humidityQuality = MeasurementQuality::Good;
    SensorId id() const override { return 4; }
    const char* type() const override { return "Combined"; }
    SensorProvenance provenance() const override { return SensorProvenance::Simulated; }
    SensorState state() const override { return SensorState::Ready; }
    bool supports(MeasurementType t) const override {
        return (t == MeasurementType::Temperature && temperatureSupported)
            || (t == MeasurementType::RelativeHumidity && humiditySupported);
    }
    void begin() override {}
    SensorOperationResult service(IMeasurementSink&) override { return SensorOperationResult::NoData; }
    SensorOperationResult sample(IMeasurementSink& sink) override {
        for (int i = 0; i < 2; ++i) {
            const bool temperature = (i == 0) != reverse;
            if (temperature ? !emitTemperature : !emitHumidity) continue;
            Measurement m;
            m.type = temperature ? MeasurementType::Temperature : MeasurementType::RelativeHumidity;
            m.valid = valid;
            m.quality = valid ? (temperature ? temperatureQuality : humidityQuality) : MeasurementQuality::Degraded;
            if (valid) m.value = MeasurementValue::floatingPoint(temperature ? 20 : 50);
            sink.emit(m);
        }
        return SensorOperationResult::Completed;
    }
};
float value(const Measurement& m) { float f = 0; TEST_ASSERT_TRUE(m.value.tryGetFloatingPoint(f)); return f; }
void setUp() {}
void tearDown() {}
void test_humidity_reference_and_boundaries() {
    float dew=0, absolute=0; bool hasDew=false;
    TEST_ASSERT_TRUE(humidityDerivedValues(20, 50, dew, hasDew, absolute));
    TEST_ASSERT_TRUE(hasDew);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 9.255, dew);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 8.621, absolute);
    TEST_ASSERT_TRUE(humidityDerivedValues(-20, 100, dew, hasDew, absolute));
    TEST_ASSERT_FLOAT_WITHIN(0.001, -20, dew);
    TEST_ASSERT_TRUE(humidityDerivedValues(20, 0, dew, hasDew, absolute));
    TEST_ASSERT_FALSE(hasDew); TEST_ASSERT_EQUAL_FLOAT(0, absolute);
    TEST_ASSERT_FALSE(humidityDerivedValues(20, 101, dew, hasDew, absolute));
    TEST_ASSERT_FALSE(humidityDerivedValues(61, 50, dew, hasDew, absolute));
    TEST_ASSERT_FALSE(humidityDerivedValues(NAN, 50, dew, hasDew, absolute));
}
void test_pair_is_from_same_operation_and_instance() {
    Combined sensor; HumidityDerivedSensor derived; derived.bind(sensor); Capture output;
    derived.sample(output);
    TEST_ASSERT_EQUAL_UINT32(4, output.values.size());
    TEST_ASSERT_FLOAT_WITHIN(0.01, 9.255, value(output.values[2]));
    sensor.reverse=true; output.values.clear(); derived.sample(output);
    TEST_ASSERT_TRUE(output.values[2].valid);
    // A previous operation's humidity must not be reused.
    sensor.emitHumidity=false; output.values.clear(); derived.sample(output);
    TEST_ASSERT_EQUAL_UINT32(3, output.values.size());
    TEST_ASSERT_FALSE(output.values[1].valid); TEST_ASSERT_FALSE(output.values[2].valid);
    sensor.emitHumidity=true; sensor.emitTemperature=false;
    output.values.clear(); derived.sample(output); TEST_ASSERT_FALSE(output.values[1].valid);
    Combined separate; separate.temperatureSupported=false;
    HumidityDerivedSensor humidityOnly; humidityOnly.bind(separate);
    TEST_ASSERT_FALSE(humidityOnly.supports(MeasurementType::DewPoint));
    output.values.clear(); humidityOnly.sample(output);
    TEST_ASSERT_EQUAL_UINT32(2, output.values.size()); // no derived channels
    sensor.emitTemperature=true; sensor.valid=false;
    output.values.clear(); derived.sample(output); TEST_ASSERT_FALSE(output.values[2].valid);
}
void test_humidity_quality_preserves_both_inputs() {
    Combined sensor; HumidityDerivedSensor derived; derived.bind(sensor); Capture output;
    const MeasurementQuality qualities[] = {MeasurementQuality::Good, MeasurementQuality::Estimated,
        MeasurementQuality::BelowMeasurementRange, MeasurementQuality::Degraded};
    // All pairs, including both input orders; enum numeric order is not severity order.
    for (size_t t = 0; t < 4; ++t) {
        for (size_t h = 0; h < 4; ++h) {
            sensor.temperatureQuality = qualities[t]; sensor.humidityQuality = qualities[h];
            output.values.clear(); derived.sample(output);
            const auto expected = qualities[t > h ? t : h];
            for (size_t i = 2; i < 4; ++i) {
                TEST_ASSERT_TRUE(output.values[i].valid);
                TEST_ASSERT_EQUAL_INT(static_cast<int>(expected),static_cast<int>(output.values[i].quality));
            }
        }
    }
    sensor.valid = false; output.values.clear(); derived.sample(output);
    TEST_ASSERT_FALSE(output.values[2].valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementQuality::Degraded),static_cast<int>(output.values[2].quality));
}
void test_pipeline_cache_property_mqtt_and_presync() {
    Combined sensor; HumidityDerivedSensor derived; derived.bind(sensor);
    TestConfigurationService config; TestTimeService time; TestMqttService mqtt; TestClock clock;
    MeasurementSnapshotCache cache; MeasurementPublisher publisher(config,time,mqtt);
    SensorManager manager(time,clock,publisher,cache);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorRegistrationResult::Registered),
        static_cast<int>(manager.registerSensor(derived, SensorSchedule::periodic(1000))));
    manager.begin(); manager.loop();
    MeasurementSnapshot snapshot;
    TEST_ASSERT_TRUE(cache.snapshot(4,MeasurementType::DewPoint,snapshot));
    TEST_ASSERT_EQUAL_UINT32(time.utc,snapshot.measurement.timestamp);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementQuality::Good),static_cast<int>(snapshot.measurement.quality));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorProvenance::Simulated),static_cast<int>(snapshot.measurement.provenance));
    TEST_ASSERT_EQUAL_UINT32(4,mqtt.messages.size());
    TEST_ASSERT_NOT_NULL(strstr(mqtt.messages[2].topic.c_str(),"/sensor/4/dew_point"));
    TEST_ASSERT_NOT_NULL(strstr(mqtt.messages[2].payload.c_str(),"\"derived\":true"));
    TEST_ASSERT_NOT_NULL(strstr(mqtt.messages[2].payload.c_str(),"\"quality\":\"good\""));
    SensorPropertyReader reader(derived,cache); PropertyDescription description;
    TEST_ASSERT_TRUE(reader.describe(PropertyReference(PropertyComponentKind::Sensor,4,"absolute_humidity"),description));
    time.synced=false; clock.milliseconds=1000; manager.loop();
    TEST_ASSERT_EQUAL_UINT32(4,mqtt.messages.size());
}
GeoPosition berlin() {
    GeoPosition p; p.latitude.configured=p.longitude.configured=true;
    p.latitude.value=52.52; p.longitude.value=13.405; return p;
}
void test_solar_reference_night_and_invalid_location() {
    float az=0,el=0; GeoPosition p=berlin();
    TEST_ASSERT_TRUE(solarPosition(p,1718971200,az,el)); // June solstice, Berlin, 12 UTC
    TEST_ASSERT_FLOAT_WITHIN(0.3,203.7,az); TEST_ASSERT_FLOAT_WITHIN(0.3,59.3,el);
    TEST_ASSERT_TRUE(solarPosition(p,1718928000,az,el)); TEST_ASSERT_TRUE(el < 0);
    TEST_ASSERT_TRUE(az>=0 && az<360);
    // Published NREL SPA example: 2003-10-17 19:30:30 UTC, Colorado.
    p.latitude.value=39.742476; p.longitude.value=-105.1786;
    TEST_ASSERT_TRUE(solarPosition(p,1066419030,az,el));
    TEST_ASSERT_FLOAT_WITHIN(0.1,194.34,az);
    TEST_ASSERT_FLOAT_WITHIN(0.1,39.872,el); // geometric, without atmospheric refraction
    p=berlin(); p.latitude.value=90; TEST_ASSERT_TRUE(solarPosition(p,1718971200,az,el));
    TEST_ASSERT_FLOAT_WITHIN(0.1,23.44,el);
    p.longitude.configured=false; TEST_ASSERT_FALSE(solarPosition(p,1718971200,az,el));
    p=berlin(); p.latitude.value=91; TEST_ASSERT_FALSE(solarPosition(p,1718971200,az,el));
}
void test_solar_sensor_validity_recovery_and_canonical_units() {
    TestConfigurationService config; TestTimeService time; Capture output;
    SolarPositionSensor solar(16,config,time); solar.begin(); solar.sample(output);
    TEST_ASSERT_FALSE(output.values[0].valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementQuality::Degraded),static_cast<int>(output.values[0].quality));
    config.configuration.geoPosition=berlin(); output.values.clear(); solar.sample(output);
    TEST_ASSERT_EQUAL_UINT32(2,output.values.size()); TEST_ASSERT_TRUE(output.values[1].valid);
    for (const auto& m : output.values) {
        TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementQuality::Good),static_cast<int>(m.quality));
    }
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorState::Ready),static_cast<int>(solar.state()));
    time.synced=false; output.values.clear(); solar.sample(output); TEST_ASSERT_FALSE(output.values[0].valid);
    float converted=0;
    TEST_ASSERT_TRUE(UnitConverter::convert(MeasurementType::DewPoint,0,PresentationUnit::DegreeFahrenheit,converted));
    TEST_ASSERT_EQUAL_FLOAT(32,converted);
    PresentationUnit unit;
    TEST_ASSERT_TRUE(UnitConverter::parseStableKey("gram_per_cubic_metre",unit));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PresentationUnit::GramPerCubicMetre),static_cast<int>(unit));
}
int main(int,char**) {
    UNITY_BEGIN();
    RUN_TEST(test_humidity_reference_and_boundaries);
    RUN_TEST(test_pair_is_from_same_operation_and_instance);
    RUN_TEST(test_humidity_quality_preserves_both_inputs);
    RUN_TEST(test_pipeline_cache_property_mqtt_and_presync);
    RUN_TEST(test_solar_reference_night_and_invalid_location);
    RUN_TEST(test_solar_sensor_validity_recovery_and_canonical_units);
    return UNITY_END();
}
