#include <unity.h>

#include <string>
#include <vector>

#include "ActuatorMqttAdapter.h"
#include "ActuatorStatePublisher.h"
#include "MqttTopic.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }

int gpioModes[256] = {};
int gpioValues[256] = {};
void pinMode(unsigned char pin, int mode) { gpioModes[pin] = mode; }
void digitalWrite(unsigned char pin, int value) { gpioValues[pin] = value; }

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
    bool resetToDefaults() override { return false; }
};

struct PublishedMessage {
    std::string topic;
    std::string payload;
    bool retained;
};

class TestMqttService : public IMqttService {
public:
    bool isConnected = true;
    unsigned int subscribeCount = 0;
    std::string subscribedTopic;
    IMqttMessageHandler* handler = nullptr;
    std::vector<PublishedMessage> messages;

    void begin() override {}
    void loop() override {}
    bool connected() const override { return isConnected; }
    bool publish(const char* topic, const char* payload, bool retained) override {
        if (!isConnected) return false;
        messages.push_back({topic, payload, retained});
        return true;
    }
    bool subscribe(const char* topic) override {
        if (!isConnected) return false;
        ++subscribeCount;
        subscribedTopic = topic;
        return true;
    }
    void setMessageHandler(IMqttMessageHandler* value) override { handler = value; }
};

void initializeSlots(ActuatorSlotConfiguration* slots) {
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        slots[index].slotId = static_cast<ActuatorId>(index + 1);
        slots[index].name = "Unused";
        slots[index].enabled = false;
        slots[index].implementation = ActuatorImplementation::None;
        slots[index].hardware = HardwareResourceAssignment::none();
    }
}

void configureSlot(ActuatorSlotConfiguration& slot, uint8_t gpio) {
    slot.enabled = true;
    slot.name = "Test";
    slot.implementation = ActuatorImplementation::GpioOnOff;
    slot.hardware = HardwareResourceAssignment::gpioResource(GpioResource(gpio));
}

struct Fixture {
    TestLogger logger;
    TestConfigurationService configuration;
    TestMqttService mqtt;
    ActuatorFactory factory;
    ActuatorRuntime runtime;
    ActuatorMqttAdapter adapter;
    ActuatorStatePublisher publisher;
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];

    Fixture()
        : factory(logger)
        , runtime(factory, logger)
        , adapter(logger, configuration, mqtt, runtime)
        , publisher(logger, configuration, mqtt, runtime) {
        configuration.configuration.device.name = "Weather Station";
        initializeSlots(slots);
    }

    void initialize() {
        runtime.initialize(slots);
        adapter.begin();
    }
};

void test_actuator_topics_generate_and_parse_slot_id() {
    const String device("Weather Station");
    TEST_ASSERT_EQUAL_STRING(
        "envnode/Weather_Station/actuator/+/cmd/on_off",
        mqttActuatorCommandSubscription(device).c_str());
    TEST_ASSERT_EQUAL_STRING(
        "envnode/Weather_Station/actuator/2/cmd/on_off",
        mqttActuatorCommandTopic(device, 2).c_str());
    TEST_ASSERT_EQUAL_STRING(
        "envnode/Weather_Station/actuator/2/status/on_off",
        mqttActuatorStatusTopic(device, 2).c_str());
    ActuatorId id = InvalidActuatorId;
    TEST_ASSERT_TRUE(parseMqttActuatorCommandTopic(
        "envnode/Weather_Station/actuator/2/cmd/on_off", device, id));
    TEST_ASSERT_EQUAL_UINT16(2, id);
    TEST_ASSERT_FALSE(parseMqttActuatorCommandTopic(
        "envnode/Weather_Station/actuator/x/cmd/on_off", device, id));
}

void test_commands_map_exact_payloads_to_correct_actuators() {
    Fixture fixture;
    configureSlot(fixture.slots[0], 16);
    configureSlot(fixture.slots[1], 17);
    fixture.initialize();

    fixture.adapter.handleMqttMessage(
        "envnode/Weather_Station/actuator/2/cmd/on_off",
        reinterpret_cast<const uint8_t*>("ON"), 2);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off),
        static_cast<int>(fixture.runtime.onOffActuator(1)->state()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On),
        static_cast<int>(fixture.runtime.onOffActuator(2)->state()));

    fixture.adapter.handleMqttMessage(
        "envnode/Weather_Station/actuator/2/cmd/on_off",
        reinterpret_cast<const uint8_t*>("OFF"), 3);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off),
        static_cast<int>(fixture.runtime.onOffActuator(2)->state()));
}

void test_invalid_payload_and_actuator_id_are_rejected() {
    Fixture fixture;
    configureSlot(fixture.slots[0], 16);
    fixture.initialize();
    fixture.adapter.handleMqttMessage(
        "envnode/Weather_Station/actuator/1/cmd/on_off",
        reinterpret_cast<const uint8_t*>("on"), 2);
    fixture.adapter.handleMqttMessage(
        "envnode/Weather_Station/actuator/16/cmd/on_off",
        reinterpret_cast<const uint8_t*>("ON"), 2);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off),
        static_cast<int>(fixture.runtime.onOffActuator(1)->state()));
}

void test_subscription_is_restored_after_reconnect() {
    Fixture fixture;
    fixture.initialize();
    fixture.adapter.loop();
    TEST_ASSERT_EQUAL_UINT32(1, fixture.mqtt.subscribeCount);
    fixture.adapter.loop();
    TEST_ASSERT_EQUAL_UINT32(1, fixture.mqtt.subscribeCount);
    fixture.mqtt.isConnected = false;
    fixture.adapter.loop();
    fixture.mqtt.isConnected = true;
    fixture.adapter.loop();
    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.subscribeCount);
}

void test_status_is_retained_and_observes_non_mqtt_state_change() {
    Fixture fixture;
    configureSlot(fixture.slots[0], 16);
    fixture.initialize();
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(1, fixture.mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("OFF", fixture.mqtt.messages[0].payload.c_str());
    TEST_ASSERT_TRUE(fixture.mqtt.messages[0].retained);

    fixture.runtime.onOffActuator(1)->setState(OnOffState::On);
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("ON", fixture.mqtt.messages[1].payload.c_str());
    TEST_ASSERT_TRUE(fixture.mqtt.messages[1].retained);
}

void test_status_republishes_after_reconnect_and_clears_removed_slot() {
    Fixture fixture;
    configureSlot(fixture.slots[0], 16);
    fixture.initialize();
    fixture.publisher.loop();
    fixture.mqtt.isConnected = false;
    fixture.publisher.loop();
    fixture.mqtt.isConnected = true;
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("OFF", fixture.mqtt.messages[1].payload.c_str());

    fixture.slots[0].enabled = false;
    TEST_ASSERT_TRUE(fixture.runtime.rebuild(fixture.slots));
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(3, fixture.mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("", fixture.mqtt.messages[2].payload.c_str());
    TEST_ASSERT_TRUE(fixture.mqtt.messages[2].retained);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_actuator_topics_generate_and_parse_slot_id);
    RUN_TEST(test_commands_map_exact_payloads_to_correct_actuators);
    RUN_TEST(test_invalid_payload_and_actuator_id_are_rejected);
    RUN_TEST(test_subscription_is_restored_after_reconnect);
    RUN_TEST(test_status_is_retained_and_observes_non_mqtt_state_change);
    RUN_TEST(test_status_republishes_after_reconnect_and_clears_removed_slot);
    return UNITY_END();
}
