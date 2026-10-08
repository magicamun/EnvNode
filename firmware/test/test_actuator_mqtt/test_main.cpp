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

void configureLevelSlot(ActuatorSlotConfiguration& slot, uint8_t gpio) {
    slot.enabled = true;
    slot.name = "Level Test";
    slot.implementation = ActuatorImplementation::GpioPwm;
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
    TEST_ASSERT_EQUAL_STRING(
        "envnode/Weather_Station/actuator/+/cmd/level",
        mqttActuatorLevelCommandSubscription(device).c_str());
    TEST_ASSERT_EQUAL_STRING(
        "envnode/Weather_Station/actuator/2/cmd/level",
        mqttActuatorLevelCommandTopic(device, 2).c_str());
    TEST_ASSERT_EQUAL_STRING(
        "envnode/Weather_Station/actuator/2/status/level",
        mqttActuatorLevelStatusTopic(device, 2).c_str());
    ActuatorId id = InvalidActuatorId;
    TEST_ASSERT_TRUE(parseMqttActuatorCommandTopic(
        "envnode/Weather_Station/actuator/2/cmd/on_off", device, id));
    TEST_ASSERT_EQUAL_UINT16(2, id);
    TEST_ASSERT_FALSE(parseMqttActuatorCommandTopic(
        "envnode/Weather_Station/actuator/x/cmd/on_off", device, id));
    TEST_ASSERT_TRUE(parseMqttActuatorLevelCommandTopic(
        "envnode/Weather_Station/actuator/2/cmd/level", device, id));
    TEST_ASSERT_EQUAL_UINT16(2, id);
}

void test_measurement_topics_cover_current_sensor_measurement_compositions() {
    const String device("Weather Station");
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/sensor/1/temperature",
        mqttMeasurementTopic(device, 1, MeasurementType::Temperature).c_str());

    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/sensor/2/temperature",
        mqttMeasurementTopic(device, 2, MeasurementType::Temperature).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/sensor/2/relative_humidity",
        mqttMeasurementTopic(device, 2, MeasurementType::RelativeHumidity).c_str());

    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/sensor/3/temperature",
        mqttMeasurementTopic(device, 3, MeasurementType::Temperature).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/sensor/3/relative_humidity",
        mqttMeasurementTopic(device, 3, MeasurementType::RelativeHumidity).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/sensor/3/atmospheric_pressure",
        mqttMeasurementTopic(device, 3, MeasurementType::AtmosphericPressure).c_str());

    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/sensor/4/temperature",
        mqttMeasurementTopic(device, 4, MeasurementType::Temperature).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/sensor/4/relative_humidity",
        mqttMeasurementTopic(device, 4, MeasurementType::RelativeHumidity).c_str());

    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/sensor/5/rain_gauge_tip",
        mqttMeasurementTopic(device, 5, MeasurementType::RainGaugeTip).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/sensor/5/rainfall_increment",
        mqttMeasurementTopic(device, 5, MeasurementType::RainfallIncrement).c_str());
    TEST_ASSERT_TRUE(mqttMeasurementTopic(
        device, InvalidSensorId, MeasurementType::Temperature).isEmpty());
    TEST_ASSERT_TRUE(mqttMeasurementTopic(
        device, 1, MeasurementType::Unknown).isEmpty());
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
    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.subscribeCount);
    fixture.adapter.loop();
    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.subscribeCount);
    fixture.mqtt.isConnected = false;
    fixture.adapter.loop();
    fixture.mqtt.isConnected = true;
    fixture.adapter.loop();
    TEST_ASSERT_EQUAL_UINT32(4, fixture.mqtt.subscribeCount);
}

void test_level_commands_are_validated_and_on_off_maps_to_boundaries() {
    Fixture fixture;
    configureLevelSlot(fixture.slots[0], 16);
    fixture.initialize();
    ILevelActuator* actuator = fixture.runtime.levelActuator(1);
    TEST_ASSERT_NOT_NULL(actuator);

    fixture.adapter.handleMqttMessage(
        "envnode/Weather_Station/actuator/1/cmd/level",
        reinterpret_cast<const uint8_t*>("37"), 2);
    TEST_ASSERT_EQUAL_UINT8(37, actuator->level().percent());
    fixture.adapter.handleMqttMessage(
        "envnode/Weather_Station/actuator/1/cmd/level",
        reinterpret_cast<const uint8_t*>("101"), 3);
    TEST_ASSERT_EQUAL_UINT8(37, actuator->level().percent());
    fixture.adapter.handleMqttMessage(
        "envnode/Weather_Station/actuator/1/cmd/on_off",
        reinterpret_cast<const uint8_t*>("OFF"), 3);
    TEST_ASSERT_EQUAL_UINT8(0, actuator->level().percent());
    fixture.adapter.handleMqttMessage(
        "envnode/Weather_Station/actuator/1/cmd/on_off",
        reinterpret_cast<const uint8_t*>("ON"), 2);
    TEST_ASSERT_EQUAL_UINT8(100, actuator->level().percent());
}

void test_level_state_publishes_retained_level_and_derived_on_off() {
    Fixture fixture;
    configureLevelSlot(fixture.slots[0], 16);
    fixture.initialize();
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("OFF", fixture.mqtt.messages[0].payload.c_str());
    TEST_ASSERT_EQUAL_STRING("0", fixture.mqtt.messages[1].payload.c_str());
    TEST_ASSERT_TRUE(fixture.mqtt.messages[1].retained);

    ActuatorLevel partial;
    TEST_ASSERT_TRUE(ActuatorLevel::tryCreate(50, partial));
    fixture.runtime.levelActuator(1)->setLevel(partial);
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(4, fixture.mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("ON", fixture.mqtt.messages[2].payload.c_str());
    TEST_ASSERT_EQUAL_STRING("50", fixture.mqtt.messages[3].payload.c_str());

    fixture.mqtt.isConnected = false;
    fixture.publisher.loop();
    fixture.mqtt.isConnected = true;
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(6, fixture.mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("ON", fixture.mqtt.messages[4].payload.c_str());
    TEST_ASSERT_EQUAL_STRING("50", fixture.mqtt.messages[5].payload.c_str());

    fixture.slots[0].enabled = false;
    TEST_ASSERT_TRUE(fixture.runtime.rebuild(fixture.slots));
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(8, fixture.mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("", fixture.mqtt.messages[6].payload.c_str());
    TEST_ASSERT_EQUAL_STRING("", fixture.mqtt.messages[7].payload.c_str());
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
    RUN_TEST(test_measurement_topics_cover_current_sensor_measurement_compositions);
    RUN_TEST(test_commands_map_exact_payloads_to_correct_actuators);
    RUN_TEST(test_invalid_payload_and_actuator_id_are_rejected);
    RUN_TEST(test_subscription_is_restored_after_reconnect);
    RUN_TEST(test_level_commands_are_validated_and_on_off_maps_to_boundaries);
    RUN_TEST(test_level_state_publishes_retained_level_and_derived_on_off);
    RUN_TEST(test_status_is_retained_and_observes_non_mqtt_state_change);
    RUN_TEST(test_status_republishes_after_reconnect_and_clears_removed_slot);
    return UNITY_END();
}
