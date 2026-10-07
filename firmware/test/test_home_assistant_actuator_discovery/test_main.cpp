#include <Arduino.h>
#include <unity.h>

#include <cstring>
#include <string>
#include <vector>

#include "ActuatorRuntime.h"
#include "HomeAssistantDiscoveryPublisher.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

class TestLogger : public ILogger {
public:
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
};

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
    bool synchronized() const override { return false; }
    time_t now() const override { return 0; }
    bool localCivilTime(tm&) const override { return false; }
    String iso8601Utc() const override { return String(); }
    String iso8601Local() const override { return String(); }
    String iso8601Local(time_t) const override { return String(); }
    uint32_t epoch() const override { return 0; }
};

class TestClock : public IMonotonicClock {
public:
    uint32_t nowMs() const override { return 0; }
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

struct Fixture {
    TestLogger logger;
    TestConfigurationService configuration;
    TestMqttService mqtt;
    TestTimeService time;
    TestClock clock;
    TestMeasurementSink sink;
    TestMeasurementObserver observer;
    SensorManager sensors;
    ActuatorFactory factory;
    ActuatorRuntime actuators;
    HomeAssistantDiscoveryPublisher publisher;

    Fixture()
        : sensors(time, clock, sink, observer)
        , factory(logger)
        , actuators(factory, logger)
        , publisher(logger, configuration, mqtt, sensors, actuators) {
        configuration.configuration.device.name = "Weather Station";
        for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
            ActuatorSlotConfiguration& slot = configuration.configuration.actuatorSlots[index];
            slot.slotId = static_cast<ActuatorId>(index + 1);
            slot.name = String("Actuator Slot ") + String(static_cast<unsigned int>(index + 1));
        }
    }

    void configure(
        size_t index,
        const char* name,
        ActuatorImplementation implementation,
        uint8_t gpio) {
        ActuatorSlotConfiguration& slot = configuration.configuration.actuatorSlots[index];
        slot.enabled = true;
        slot.name = name;
        slot.implementation = implementation;
        slot.hardware = HardwareResourceAssignment::gpioResource(GpioResource(gpio));
    }

    const std::string& publish() {
        TEST_ASSERT_TRUE(actuators.rebuild(configuration.configuration.actuatorSlots));
        TEST_ASSERT_EQUAL_INT(
            static_cast<int>(DiscoveryRepublishResult::Published),
            static_cast<int>(publisher.republish()));
        return mqtt.messages.back().payload;
    }
};

void assertContains(const std::string& text, const char* expected) {
    TEST_ASSERT_NOT_NULL(strstr(text.c_str(), expected));
}

void test_on_off_actuator_is_discovered_as_switch() {
    Fixture fixture;
    fixture.actuators.initialize(fixture.configuration.configuration.actuatorSlots);
    fixture.configure(0, "Irrigation Valve", ActuatorImplementation::GpioOnOff, 16);

    const std::string& payload = fixture.publish();

    assertContains(payload, "\"weatherstation_001122334455_actuator_1\":{\"p\":\"switch\"");
    assertContains(payload, "\"name\":\"Irrigation Valve\"");
    assertContains(payload, "\"command_topic\":\"envnode/Weather_Station/actuator/1/cmd/on_off\"");
    assertContains(payload, "\"state_topic\":\"envnode/Weather_Station/actuator/1/status/on_off\"");
    TEST_ASSERT_NULL(strstr(payload.c_str(), "brightness_command_topic"));
    TEST_ASSERT_EQUAL_UINT32(1, fixture.publisher.lastEntityCount());
}

void test_level_actuator_is_one_light_with_zero_to_one_hundred_brightness() {
    Fixture fixture;
    fixture.actuators.initialize(fixture.configuration.configuration.actuatorSlots);
    fixture.configure(2, "Ventilation", ActuatorImplementation::GpioPwm, 17);

    const std::string& payload = fixture.publish();

    assertContains(payload, "\"weatherstation_001122334455_actuator_3\":{\"p\":\"light\"");
    assertContains(payload, "\"payload_on\":\"ON\",\"payload_off\":\"OFF\"");
    assertContains(payload, "\"on_command_type\":\"first\"");
    assertContains(payload, "\"brightness_command_topic\":\"envnode/Weather_Station/actuator/3/cmd/level\"");
    assertContains(payload, "\"brightness_state_topic\":\"envnode/Weather_Station/actuator/3/status/level\"");
    assertContains(payload, "\"brightness_scale\":100");
    TEST_ASSERT_EQUAL_UINT32(1, fixture.publisher.lastEntityCount());
}

void test_disabled_actuator_is_removed_from_discovery() {
    Fixture fixture;
    fixture.actuators.initialize(fixture.configuration.configuration.actuatorSlots);
    fixture.configure(0, "Valve", ActuatorImplementation::GpioOnOff, 16);
    fixture.publish();
    fixture.mqtt.messages.clear();

    fixture.configuration.configuration.actuatorSlots[0].enabled = false;
    const std::string& payload = fixture.publish();

    TEST_ASSERT_NULL(strstr(payload.c_str(), "_actuator_1"));
    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.messages.size());
    assertContains(
        fixture.mqtt.messages[0].payload,
        "\"weatherstation_001122334455_actuator_1\":{\"p\":\"switch\"}");
    TEST_ASSERT_EQUAL_UINT32(0, fixture.publisher.lastEntityCount());
}

void test_platform_change_publishes_old_platform_tombstone_then_new_entity() {
    Fixture fixture;
    fixture.actuators.initialize(fixture.configuration.configuration.actuatorSlots);
    fixture.configure(0, "Output", ActuatorImplementation::GpioOnOff, 16);
    fixture.publish();
    fixture.mqtt.messages.clear();

    fixture.configure(0, "Output", ActuatorImplementation::GpioPwm, 16);
    fixture.publish();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.messages.size());
    assertContains(
        fixture.mqtt.messages[0].payload,
        "\"weatherstation_001122334455_actuator_1\":{\"p\":\"switch\"}");
    assertContains(
        fixture.mqtt.messages[1].payload,
        "\"weatherstation_001122334455_actuator_1\":{\"p\":\"light\"");
}

} // namespace

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_on_off_actuator_is_discovered_as_switch);
    RUN_TEST(test_level_actuator_is_one_light_with_zero_to_one_hundred_brightness);
    RUN_TEST(test_disabled_actuator_is_removed_from_discovery);
    RUN_TEST(test_platform_change_publishes_old_platform_tombstone_then_new_entity);
    return UNITY_END();
}
