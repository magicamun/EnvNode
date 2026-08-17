#include <Arduino.h>
#include <unity.h>

#include <cstring>
#include <string>
#include <vector>

#include "ExternalDescriptionBuilder.h"
#include "MqttDescriptionPublisher.h"
#include "MqttTopic.h"

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

class TestClock : public IMonotonicClock {
public:
    uint32_t now = 0;
    uint32_t nowMs() const override { return now; }
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
    bool failNext = false;
    std::vector<PublishedMessage> messages;
    void begin() override {}
    void loop() override {}
    bool connected() const override { return isConnected; }
    bool publish(const char* topic, const char* payload, bool retained) override {
        if (!isConnected || failNext) {
            failNext = false;
            return false;
        }
        messages.push_back({topic, payload, retained});
        return true;
    }
    bool subscribe(const char*) override { return true; }
    void setMessageHandler(IMqttMessageHandler*) override {}
};

struct Fixture {
    TestLogger logger;
    TestClock clock;
    TestConfigurationService configuration;
    TestMqttService mqtt;
    MqttDescriptionPublisher publisher;

    Fixture()
        : publisher(logger, configuration, mqtt, clock) {
        configuration.configuration.device.name = "Weather Station";
        for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
            configuration.configuration.actuatorSlots[index].slotId =
                static_cast<ActuatorId>(index + 1);
        }
        for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
            configuration.configuration.controllerSlots[index].slotId =
                static_cast<ControllerId>(index + 1);
        }
    }
};

const PublishedMessage* findLast(
    const std::vector<PublishedMessage>& messages,
    const char* suffix) {
    for (size_t offset = 0; offset < messages.size(); ++offset) {
        const PublishedMessage& message = messages[messages.size() - 1 - offset];
        if (message.topic.size() >= strlen(suffix)
            && message.topic.compare(message.topic.size() - strlen(suffix),
                strlen(suffix), suffix) == 0) return &message;
    }
    return nullptr;
}

void configureActuator(ActuatorSlotConfiguration& slot, bool enabled = true) {
    slot.enabled = enabled;
    slot.name = "Heater";
    slot.implementation = ActuatorImplementation::GpioOnOff;
}

void configureBlink(ControllerSlotConfiguration& slot, bool enabled = true) {
    slot.enabled = enabled;
    slot.name = "Cycle";
    slot.implementation = ControllerImplementation::Blink;
    slot.implementationConfiguration.blink.targetActuatorId = 1;
    slot.implementationConfiguration.blink.onDurationMs = 1000;
    slot.implementationConfiguration.blink.offDurationMs = 500;
}

void test_description_topics_preserve_existing_hierarchy() {
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/actuator/2/description",
        mqttActuatorDescriptionTopic("Weather Station", 2).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/3/description",
        mqttControllerDescriptionTopic("Weather Station", 3).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/actuator/2/cmd/on_off",
        mqttActuatorCommandTopic("Weather Station", 2).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/3/cmd",
        mqttControllerCommandTopic("Weather Station", 3).c_str());
}

void test_initial_connect_reconciles_all_slots_and_describes_disabled_slots() {
    Fixture fixture;
    configureActuator(fixture.configuration.configuration.actuatorSlots[0], false);
    configureBlink(fixture.configuration.configuration.controllerSlots[0], false);
    fixture.publisher.loop();

    TEST_ASSERT_EQUAL_UINT32(MaxActuatorSlotCount + MaxControllerSlotCount,
        fixture.mqtt.messages.size());
    const PublishedMessage* actuator = findLast(fixture.mqtt.messages,
        "/actuator/1/description");
    const PublishedMessage* controller = findLast(fixture.mqtt.messages,
        "/controller/1/description");
    const PublishedMessage* none = findLast(fixture.mqtt.messages,
        "/controller/16/description");
    TEST_ASSERT_NOT_NULL(actuator);
    TEST_ASSERT_NOT_NULL(controller);
    TEST_ASSERT_NOT_NULL(none);
    TEST_ASSERT_TRUE(actuator->retained);
    TEST_ASSERT_NOT_NULL(strstr(actuator->payload.c_str(), "\"enabled\":false"));
    TEST_ASSERT_NOT_NULL(strstr(controller->payload.c_str(), "\"enabled\":false"));
    TEST_ASSERT_EQUAL_STRING("", none->payload.c_str());

    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(MaxActuatorSlotCount + MaxControllerSlotCount,
        fixture.mqtt.messages.size());
}

void test_reconnect_forces_complete_reconciliation() {
    Fixture fixture;
    fixture.publisher.loop();
    fixture.mqtt.messages.clear();
    fixture.mqtt.isConnected = false;
    fixture.publisher.loop();
    fixture.mqtt.isConnected = true;
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(MaxActuatorSlotCount + MaxControllerSlotCount,
        fixture.mqtt.messages.size());
}

void test_configuration_changes_publish_but_unchanged_configuration_does_not() {
    Fixture fixture;
    configureActuator(fixture.configuration.configuration.actuatorSlots[0]);
    configureBlink(fixture.configuration.configuration.controllerSlots[0]);
    fixture.publisher.loop();
    fixture.mqtt.messages.clear();

    fixture.configuration.configuration.actuatorSlots[0].name = "Pump";
    fixture.configuration.configuration.controllerSlots[0]
        .implementationConfiguration.blink.onDurationMs = 2500;
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.messages.size());
    TEST_ASSERT_NOT_NULL(strstr(fixture.mqtt.messages[0].payload.c_str(), "Pump"));
    TEST_ASSERT_NOT_NULL(strstr(fixture.mqtt.messages[1].payload.c_str(),
        "\"value\":2500"));

    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.messages.size());

    fixture.mqtt.messages.clear();
    fixture.configuration.configuration.actuatorSlots[0].enabled = false;
    ControllerSlotConfiguration& controller =
        fixture.configuration.configuration.controllerSlots[0];
    controller.implementation = ControllerImplementation::Threshold;
    controller.implementationConfiguration.threshold.source =
        MeasurementSourceReference(2, MeasurementType::RelativeHumidity);
    controller.implementationConfiguration.threshold.targetActuatorId = 2;
    controller.implementationConfiguration.threshold.onThreshold = 70.0F;
    controller.implementationConfiguration.threshold.offThreshold = 65.0F;
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.messages.size());
    TEST_ASSERT_NOT_NULL(strstr(fixture.mqtt.messages[0].payload.c_str(),
        "\"enabled\":false"));
    TEST_ASSERT_NOT_NULL(strstr(fixture.mqtt.messages[1].payload.c_str(),
        "\"implementation\":\"threshold\""));
    TEST_ASSERT_NOT_NULL(strstr(fixture.mqtt.messages[1].payload.c_str(),
        "\"sensor_id\":2"));
    TEST_ASSERT_NOT_NULL(strstr(fixture.mqtt.messages[1].payload.c_str(),
        "\"actuator_id\":2"));
}

void test_none_transitions_clear_and_publish_retained_description() {
    Fixture fixture;
    configureBlink(fixture.configuration.configuration.controllerSlots[0]);
    fixture.publisher.loop();
    fixture.mqtt.messages.clear();

    fixture.configuration.configuration.controllerSlots[0].implementation =
        ControllerImplementation::None;
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(1, fixture.mqtt.messages.size());
    TEST_ASSERT_TRUE(fixture.mqtt.messages[0].retained);
    TEST_ASSERT_EQUAL_STRING("", fixture.mqtt.messages[0].payload.c_str());

    configureBlink(fixture.configuration.configuration.controllerSlots[0]);
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.messages.size());
    TEST_ASSERT_NOT_NULL(strstr(fixture.mqtt.messages[1].payload.c_str(),
        "\"implementation\":\"blink\""));
}

void test_failed_publication_remains_dirty_and_retries_later() {
    Fixture fixture;
    configureActuator(fixture.configuration.configuration.actuatorSlots[0]);
    fixture.mqtt.failNext = true;
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(MaxActuatorSlotCount + MaxControllerSlotCount - 1,
        fixture.mqtt.messages.size());

    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(MaxActuatorSlotCount + MaxControllerSlotCount - 1,
        fixture.mqtt.messages.size());
    fixture.clock.now = 1000;
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(MaxActuatorSlotCount + MaxControllerSlotCount,
        fixture.mqtt.messages.size());
    TEST_ASSERT_NOT_NULL(findLast(fixture.mqtt.messages, "/actuator/1/description"));
}

void test_threshold_payload_fits_current_mqtt_buffer() {
    ControllerSlotConfiguration slot;
    slot.slotId = 1;
    slot.enabled = true;
    slot.name = "12345678901234567890123456789012";
    slot.implementation = ControllerImplementation::Threshold;
    slot.implementationConfiguration.threshold.targetActuatorId = 1;
    slot.implementationConfiguration.threshold.onThreshold = 3.4028234e38F;
    slot.implementationConfiguration.threshold.offThreshold = -3.4028234e38F;
    slot.implementationConfiguration.threshold.maxMeasurementAgeMs = 2147483647UL;
    size_t maximumLength = 0;
    for (uint8_t value = 1; value <= SupportedMeasurementTypeCount; ++value) {
        const MeasurementType type = static_cast<MeasurementType>(value);
        const MeasurementTypeMetadata& metadata = measurementTypeMetadata(type);
        if (metadata.expectedValueKind != ValueKind::FloatingPoint
            || metadata.semantics != MeasurementSemantics::State) continue;
        slot.implementationConfiguration.threshold.source =
            MeasurementSourceReference(1, type);
        String payload;
        TEST_ASSERT_TRUE(buildControllerDescription(slot, payload));
        if (payload.length() > maximumLength) maximumLength = payload.length();
    }
    TEST_ASSERT_LESS_THAN_UINT32(16384, maximumLength);
}

} // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_description_topics_preserve_existing_hierarchy);
    RUN_TEST(test_initial_connect_reconciles_all_slots_and_describes_disabled_slots);
    RUN_TEST(test_reconnect_forces_complete_reconciliation);
    RUN_TEST(test_configuration_changes_publish_but_unchanged_configuration_does_not);
    RUN_TEST(test_none_transitions_clear_and_publish_retained_description);
    RUN_TEST(test_failed_publication_remains_dirty_and_retries_later);
    RUN_TEST(test_threshold_payload_fits_current_mqtt_buffer);
    return UNITY_END();
}
