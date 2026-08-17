#include <unity.h>

#include <climits>
#include <cstring>
#include <string>
#include <vector>

#include "ControllerMqttAdapter.h"
#include "ControllerStatePublisher.h"
#include "MqttMessageRouter.h"
#include "MeasurementSnapshotCache.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

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

class TestActuator : public IOnOffActuator {
public:
    OnOffState current = OnOffState::Off;
    ActuatorOperationResult begin() override { return ActuatorOperationResult::Completed; }
    ActuatorOperationResult shutdown() override { current = OnOffState::Off; return ActuatorOperationResult::Completed; }
    ActuatorOperationResult setState(OnOffState state) override { current = state; return ActuatorOperationResult::Completed; }
    OnOffState state() const override { return current; }
    bool initialized() const override { return true; }
};

class TestResolver : public IOnOffActuatorResolver {
public:
    IOnOffActuator* target = nullptr;
    IOnOffActuator* onOffActuator(ActuatorId id) override {
        return id == 1 ? target : nullptr;
    }
};

class TestConfigurationService : public IConfigurationService {
public:
    Configuration configuration;
    unsigned int controllerSetCount = 0;
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
    bool setControllerSlotConfiguration(const ControllerSlotConfiguration& slot) override {
        ++controllerSetCount;
        if (!isValidControllerId(slot.slotId) || slot.slotId > MaxControllerSlotCount
            || slot.implementation != ControllerImplementation::Blink
            || slot.implementationConfiguration.blink.onDurationMs == 0
            || slot.implementationConfiguration.blink.onDurationMs > INT32_MAX
            || slot.implementationConfiguration.blink.offDurationMs == 0
            || slot.implementationConfiguration.blink.offDurationMs > INT32_MAX) return false;
        configuration.controllerSlots[slot.slotId - 1] = slot;
        return true;
    }
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
    std::vector<std::string> subscriptions;
    std::vector<PublishedMessage> messages;
    IMqttMessageHandler* handler = nullptr;
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
        subscriptions.push_back(topic);
        return true;
    }
    void setMessageHandler(IMqttMessageHandler* value) override { handler = value; }
};

class CountingHandler : public IMqttMessageHandler {
public:
    unsigned int count = 0;
    void handleMqttMessage(const char*, const uint8_t*, size_t) override { ++count; }
};

void initializeConfiguration(Configuration& configuration) {
    configuration.device.name = "Weather Station";
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        ControllerSlotConfiguration& slot = configuration.controllerSlots[index];
        slot.slotId = static_cast<ControllerId>(index + 1);
        slot.enabled = false;
        slot.name = "Unused";
        slot.implementation = ControllerImplementation::None;
    }
    ControllerSlotConfiguration& slot = configuration.controllerSlots[0];
    slot.enabled = true;
    slot.name = "Test Blink";
    slot.implementation = ControllerImplementation::Blink;
    slot.implementationConfiguration.blink.targetActuatorId = 1;
    slot.implementationConfiguration.blink.onDurationMs = 100;
    slot.implementationConfiguration.blink.offDurationMs = 200;
}

struct Fixture {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestResolver resolver;
    MeasurementSnapshotCache measurements;
    TestConfigurationService configuration;
    TestMqttService mqtt;
    ControllerFactory factory;
    ControllerRuntime runtime;
    RuntimeManager runtimeManager;
    ControllerMqttAdapter adapter;
    ControllerStatePublisher publisher;

    Fixture()
        : factory(measurements, resolver, clock, logger)
        , runtime(factory, logger)
        , runtimeManager(logger, nullptr, nullptr, &runtime)
        , adapter(logger, configuration, mqtt, runtime, runtimeManager)
        , publisher(logger, configuration, mqtt, runtime) {
        resolver.target = &actuator;
        initializeConfiguration(configuration.configuration);
        TEST_ASSERT_TRUE(runtime.initialize(configuration.configuration.controllerSlots));
    }
};

void deliver(ControllerMqttAdapter& adapter, const char* topic, const char* payload) {
    adapter.handleMqttMessage(topic,
        reinterpret_cast<const uint8_t*>(payload), strlen(payload));
}

void test_controller_topics_generate_and_parse() {
    const String device("Weather Station");
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/+/cmd",
        mqttControllerCommandSubscription(device).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/+/cmd/parameter/+",
        mqttControllerParameterCommandSubscription(device).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/2/status",
        mqttControllerStatusTopic(device, 2).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/2/cmd",
        mqttControllerCommandTopic(device, 2).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/2/parameter/on_duration_ms",
        mqttControllerParameterTopic(device, 2, ControllerMqttParameter::OnDurationMs).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/2/cmd/parameter/on_duration_ms",
        mqttControllerParameterCommandTopic(
            device, 2, ControllerMqttParameter::OnDurationMs).c_str());
    ControllerId id = InvalidControllerId;
    ControllerMqttParameter parameter = ControllerMqttParameter::OnDurationMs;
    TEST_ASSERT_TRUE(parseMqttControllerCommandTopic(
        "envnode/Weather_Station/controller/2/cmd", device, id));
    TEST_ASSERT_EQUAL_UINT16(2, id);
    TEST_ASSERT_TRUE(parseMqttControllerParameterCommandTopic(
        "envnode/Weather_Station/controller/2/cmd/parameter/off_duration_ms",
        device, id, parameter));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerMqttParameter::OffDurationMs),
        static_cast<int>(parameter));
    TEST_ASSERT_FALSE(parseMqttControllerCommandTopic(
        "envnode/Weather_Station/controller/x/cmd", device, id));
    TEST_ASSERT_FALSE(parseMqttControllerParameterCommandTopic(
        "envnode/Weather_Station/controller/1/cmd/parameter/unknown", device, id, parameter));
    TEST_ASSERT_FALSE(parseMqttControllerParameterCommandTopic(
        "envnode/Weather_Station/controller/1/parameter/on_duration_ms",
        device, id, parameter));
}

void test_start_stop_commands_are_transient_runtime_operations() {
    Fixture fixture;
    deliver(fixture.adapter, "envnode/Weather_Station/controller/1/cmd", "STOP");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off), static_cast<int>(fixture.actuator.state()));
    ControllerRuntimeInfo info;
    TEST_ASSERT_TRUE(fixture.runtime.runtimeInfo(0, info));
    TEST_ASSERT_FALSE(info.running);
    TEST_ASSERT_TRUE(fixture.configuration.configuration.controllerSlots[0].enabled);
    fixture.clock.now = 50;
    deliver(fixture.adapter, "envnode/Weather_Station/controller/1/cmd", "START");
    TEST_ASSERT_TRUE(fixture.runtime.runtimeInfo(0, info));
    TEST_ASSERT_TRUE(info.running);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BlinkPhase::On), static_cast<int>(info.blinkPhase));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On), static_cast<int>(fixture.actuator.state()));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.configuration.controllerSetCount);
}

void test_invalid_commands_and_ids_are_rejected() {
    Fixture fixture;
    deliver(fixture.adapter, "envnode/Weather_Station/controller/1/cmd", "start");
    deliver(fixture.adapter, "envnode/Weather_Station/controller/17/cmd", "STOP");
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On), static_cast<int>(fixture.actuator.state()));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.configuration.controllerSetCount);
}

void test_parameter_update_persists_and_rebuilds_runtime() {
    Fixture fixture;
    const uint32_t revisionBefore = fixture.runtime.compositionRevision();
    deliver(fixture.adapter,
        "envnode/Weather_Station/controller/1/cmd/parameter/on_duration_ms", "250");
    TEST_ASSERT_EQUAL_UINT32(1, fixture.configuration.controllerSetCount);
    TEST_ASSERT_EQUAL_UINT32(250, fixture.configuration.configuration.controllerSlots[0]
        .implementationConfiguration.blink.onDurationMs);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(RuntimeAction::None),
        static_cast<int>(fixture.runtimeManager.pendingAction()));
    ControllerRuntimeInfo info;
    TEST_ASSERT_TRUE(fixture.runtime.runtimeInfo(0, info));
    TEST_ASSERT_EQUAL_UINT32(250, info.onDurationMs);
    TEST_ASSERT_TRUE(info.running);
    TEST_ASSERT_EQUAL_UINT32(revisionBefore + 1, fixture.runtime.compositionRevision());
}

void test_parameter_state_echo_and_unchanged_command_do_not_rebuild() {
    Fixture fixture;
    const uint32_t revisionBefore = fixture.runtime.compositionRevision();
    deliver(fixture.adapter,
        "envnode/Weather_Station/controller/1/parameter/on_duration_ms", "100");
    TEST_ASSERT_EQUAL_UINT32(0, fixture.configuration.controllerSetCount);
    TEST_ASSERT_EQUAL_UINT32(revisionBefore, fixture.runtime.compositionRevision());

    deliver(fixture.adapter,
        "envnode/Weather_Station/controller/1/cmd/parameter/on_duration_ms", "100");
    TEST_ASSERT_EQUAL_UINT32(0, fixture.configuration.controllerSetCount);
    TEST_ASSERT_EQUAL_UINT32(revisionBefore, fixture.runtime.compositionRevision());
}

void test_invalid_parameter_payloads_do_not_mutate_configuration() {
    Fixture fixture;
    const char* topic = "envnode/Weather_Station/controller/1/cmd/parameter/off_duration_ms";
    deliver(fixture.adapter, topic, "");
    deliver(fixture.adapter, topic, "-1");
    deliver(fixture.adapter, topic, "0");
    deliver(fixture.adapter, topic, "2147483648");
    deliver(fixture.adapter, topic, "12ms");
    TEST_ASSERT_EQUAL_UINT32(0, fixture.configuration.controllerSetCount);
    TEST_ASSERT_EQUAL_UINT32(200, fixture.configuration.configuration.controllerSlots[0]
        .implementationConfiguration.blink.offDurationMs);
}

void test_status_and_parameters_are_retained_and_change_detected() {
    Fixture fixture;
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(3, fixture.mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/1/status",
        fixture.mqtt.messages[0].topic.c_str());
    TEST_ASSERT_EQUAL_STRING(
        "{\"running\":true,\"phase\":\"on\",\"target_available\":true,\"last_result\":\"completed\"}",
        fixture.mqtt.messages[0].payload.c_str());
    TEST_ASSERT_TRUE(fixture.mqtt.messages[0].retained);
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(3, fixture.mqtt.messages.size());
    fixture.clock.now = 100;
    fixture.runtime.loop();
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(4, fixture.mqtt.messages.size());
    TEST_ASSERT_TRUE(fixture.mqtt.messages[3].payload.find("\"phase\":\"off\"") != std::string::npos);
}

void test_web_originated_parameter_change_is_published() {
    Fixture fixture;
    fixture.publisher.loop();
    ControllerSlotConfiguration changed = fixture.configuration.configuration.controllerSlots[0];
    changed.implementationConfiguration.blink.offDurationMs = 750;
    TEST_ASSERT_TRUE(fixture.configuration.setControllerSlotConfiguration(changed));
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(4, fixture.mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/1/parameter/off_duration_ms",
        fixture.mqtt.messages[3].topic.c_str());
    TEST_ASSERT_EQUAL_STRING("750", fixture.mqtt.messages[3].payload.c_str());
    TEST_ASSERT_TRUE(fixture.mqtt.messages[3].retained);
}

void test_reconnect_restores_subscriptions_and_republishes_state() {
    Fixture fixture;
    fixture.adapter.loop();
    TEST_ASSERT_EQUAL_UINT32(2, fixture.mqtt.subscriptions.size());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/+/cmd/parameter/+",
        fixture.mqtt.subscriptions[1].c_str());
    fixture.publisher.loop();
    fixture.mqtt.isConnected = false;
    fixture.adapter.loop();
    fixture.publisher.loop();
    fixture.mqtt.isConnected = true;
    fixture.adapter.loop();
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(4, fixture.mqtt.subscriptions.size());
    TEST_ASSERT_EQUAL_UINT32(6, fixture.mqtt.messages.size());
}

void test_runtime_rebuild_republishes_status_without_retaining_controller_pointer() {
    Fixture fixture;
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(3, fixture.mqtt.messages.size());
    TEST_ASSERT_TRUE(fixture.runtime.rebuild(
        fixture.configuration.configuration.controllerSlots));
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(6, fixture.mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/1/status",
        fixture.mqtt.messages[3].topic.c_str());
}

void test_router_delivers_to_both_independent_handlers() {
    TestMqttService mqtt;
    CountingHandler first;
    CountingHandler second;
    MqttMessageRouter router(mqtt, first, second);
    router.begin();
    TEST_ASSERT_EQUAL_PTR(&router, mqtt.handler);
    mqtt.handler->handleMqttMessage("topic", nullptr, 0);
    TEST_ASSERT_EQUAL_UINT32(1, first.count);
    TEST_ASSERT_EQUAL_UINT32(1, second.count);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_controller_topics_generate_and_parse);
    RUN_TEST(test_start_stop_commands_are_transient_runtime_operations);
    RUN_TEST(test_invalid_commands_and_ids_are_rejected);
    RUN_TEST(test_parameter_update_persists_and_rebuilds_runtime);
    RUN_TEST(test_parameter_state_echo_and_unchanged_command_do_not_rebuild);
    RUN_TEST(test_invalid_parameter_payloads_do_not_mutate_configuration);
    RUN_TEST(test_status_and_parameters_are_retained_and_change_detected);
    RUN_TEST(test_web_originated_parameter_change_is_published);
    RUN_TEST(test_reconnect_restores_subscriptions_and_republishes_state);
    RUN_TEST(test_runtime_rebuild_republishes_status_without_retaining_controller_pointer);
    RUN_TEST(test_router_delivers_to_both_independent_handlers);
    return UNITY_END();
}
