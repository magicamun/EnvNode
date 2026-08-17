#include <unity.h>

#include <climits>
#include <cmath>
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
        if (!isValidControllerId(slot.slotId) || slot.slotId > MaxControllerSlotCount) {
            return false;
        }
        if (slot.implementation == ControllerImplementation::Blink) {
            if (slot.implementationConfiguration.blink.onDurationMs == 0
                || slot.implementationConfiguration.blink.onDurationMs > INT32_MAX
                || slot.implementationConfiguration.blink.offDurationMs == 0
                || slot.implementationConfiguration.blink.offDurationMs > INT32_MAX) {
                return false;
            }
        } else if (slot.implementation == ControllerImplementation::Threshold) {
            const ThresholdControllerConfiguration& threshold =
                slot.implementationConfiguration.threshold;
            if (!std::isfinite(threshold.onThreshold)
                || !std::isfinite(threshold.offThreshold)
                || threshold.offThreshold >= threshold.onThreshold
                || threshold.maxMeasurementAgeMs == 0
                || threshold.maxMeasurementAgeMs > INT32_MAX) {
                return false;
            }
        } else {
            return false;
        }
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

    void useThreshold() {
        ControllerSlotConfiguration& slot = configuration.configuration.controllerSlots[0];
        slot.name = "Test Threshold";
        slot.implementation = ControllerImplementation::Threshold;
        ThresholdControllerConfiguration& threshold =
            slot.implementationConfiguration.threshold;
        threshold.source = MeasurementSourceReference(1, MeasurementType::Temperature);
        threshold.targetActuatorId = 1;
        threshold.onThreshold = 70.0F;
        threshold.offThreshold = 65.0F;
        threshold.maxMeasurementAgeMs = 15000;
        TEST_ASSERT_TRUE(runtime.rebuild(configuration.configuration.controllerSlots));
        configuration.controllerSetCount = 0;
        mqtt.messages.clear();
    }
};

void deliver(ControllerMqttAdapter& adapter, const char* topic, const char* payload) {
    adapter.handleMqttMessage(topic,
        reinterpret_cast<const uint8_t*>(payload), strlen(payload));
}

Measurement temperatureMeasurement(float value, bool valid = true) {
    Measurement measurement;
    measurement.source = 1;
    measurement.type = MeasurementType::Temperature;
    measurement.value = valid
        ? MeasurementValue::floatingPoint(value) : MeasurementValue::none();
    measurement.valid = valid;
    measurement.quality = MeasurementQuality::Good;
    return measurement;
}

const PublishedMessage* lastMessageForTopic(
    const TestMqttService& mqtt,
    const char* topic) {
    for (size_t index = mqtt.messages.size(); index > 0; --index) {
        if (mqtt.messages[index - 1].topic == topic) return &mqtt.messages[index - 1];
    }
    return nullptr;
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
        mqttControllerParameterTopic(device, 2, ControllerParameter::OnDurationMs).c_str());
    TEST_ASSERT_EQUAL_STRING("envnode/Weather_Station/controller/2/cmd/parameter/on_duration_ms",
        mqttControllerParameterCommandTopic(
            device, 2, ControllerParameter::OnDurationMs).c_str());
    TEST_ASSERT_EQUAL_STRING("on_threshold",
        mqttControllerParameterName(ControllerParameter::OnThreshold));
    TEST_ASSERT_EQUAL_STRING("off_threshold",
        mqttControllerParameterName(ControllerParameter::OffThreshold));
    TEST_ASSERT_EQUAL_STRING("max_measurement_age_ms",
        mqttControllerParameterName(ControllerParameter::MaxMeasurementAgeMs));
    ControllerId id = InvalidControllerId;
    ControllerParameter parameter = ControllerParameter::OnDurationMs;
    TEST_ASSERT_TRUE(parseMqttControllerCommandTopic(
        "envnode/Weather_Station/controller/2/cmd", device, id));
    TEST_ASSERT_EQUAL_UINT16(2, id);
    TEST_ASSERT_TRUE(parseMqttControllerParameterCommandTopic(
        "envnode/Weather_Station/controller/2/cmd/parameter/off_duration_ms",
        device, id, parameter));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerParameter::OffDurationMs),
        static_cast<int>(parameter));
    TEST_ASSERT_TRUE(parseMqttControllerParameterCommandTopic(
        "envnode/Weather_Station/controller/2/cmd/parameter/on_threshold",
        device, id, parameter));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerParameter::OnThreshold),
        static_cast<int>(parameter));
    TEST_ASSERT_TRUE(parseMqttControllerParameterCommandTopic(
        "envnode/Weather_Station/controller/2/cmd/parameter/max_measurement_age_ms",
        device, id, parameter));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerParameter::MaxMeasurementAgeMs),
        static_cast<int>(parameter));
    TEST_ASSERT_FALSE(parseMqttControllerCommandTopic(
        "envnode/Weather_Station/controller/x/cmd", device, id));
    TEST_ASSERT_FALSE(parseMqttControllerParameterCommandTopic(
        "envnode/Weather_Station/controller/1/cmd/parameter/unknown", device, id, parameter));
    TEST_ASSERT_FALSE(parseMqttControllerParameterCommandTopic(
        "envnode/Weather_Station/controller/1/parameter/on_duration_ms",
        device, id, parameter));
}

void test_cross_implementation_parameters_are_rejected() {
    Fixture fixture;
    deliver(fixture.adapter,
        "envnode/Weather_Station/controller/1/cmd/parameter/on_threshold", "72");
    TEST_ASSERT_EQUAL_UINT32(0, fixture.configuration.controllerSetCount);
    fixture.useThreshold();
    deliver(fixture.adapter,
        "envnode/Weather_Station/controller/1/cmd/parameter/on_duration_ms", "250");
    TEST_ASSERT_EQUAL_UINT32(0, fixture.configuration.controllerSetCount);
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

void test_threshold_float_parsing_accepts_protocol_decimals_and_rejects_invalid_text() {
    Fixture fixture;
    fixture.useThreshold();
    const char* onTopic =
        "envnode/Weather_Station/controller/1/cmd/parameter/on_threshold";
    const char* offTopic =
        "envnode/Weather_Station/controller/1/cmd/parameter/off_threshold";
    deliver(fixture.adapter, onTopic, "72");
    deliver(fixture.adapter, offTopic, "64.5");
    deliver(fixture.adapter, offTopic, "-5.25");
    TEST_ASSERT_EQUAL_UINT32(3, fixture.configuration.controllerSetCount);
    TEST_ASSERT_FLOAT_WITHIN(0.0001F, -5.25F, fixture.configuration.configuration
        .controllerSlots[0].implementationConfiguration.threshold.offThreshold);
    const uint32_t revision = fixture.runtime.compositionRevision();
    const char* invalid[] = {"", "value", "65x", "NaN", "Infinity", "-Infinity", " 65", "65,5"};
    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        deliver(fixture.adapter, offTopic, invalid[index]);
    }
    TEST_ASSERT_EQUAL_UINT32(3, fixture.configuration.controllerSetCount);
    TEST_ASSERT_EQUAL_UINT32(revision, fixture.runtime.compositionRevision());
}

void test_threshold_age_parsing_and_configuration_validation_are_strict() {
    Fixture fixture;
    fixture.useThreshold();
    const char* topic =
        "envnode/Weather_Station/controller/1/cmd/parameter/max_measurement_age_ms";
    deliver(fixture.adapter, topic, "20000");
    TEST_ASSERT_EQUAL_UINT32(1, fixture.configuration.controllerSetCount);
    TEST_ASSERT_EQUAL_UINT32(20000, fixture.configuration.configuration
        .controllerSlots[0].implementationConfiguration.threshold.maxMeasurementAgeMs);
    const uint32_t revision = fixture.runtime.compositionRevision();
    deliver(fixture.adapter, topic, "0");
    deliver(fixture.adapter, topic, "-1");
    deliver(fixture.adapter, topic, "1.5");
    deliver(fixture.adapter, topic, "2147483648");
    deliver(fixture.adapter, topic, "12ms");
    TEST_ASSERT_EQUAL_UINT32(3, fixture.configuration.controllerSetCount);
    TEST_ASSERT_EQUAL_UINT32(revision, fixture.runtime.compositionRevision());
    TEST_ASSERT_EQUAL_UINT32(20000, fixture.configuration.configuration
        .controllerSlots[0].implementationConfiguration.threshold.maxMeasurementAgeMs);
}

void test_threshold_parameter_update_rebuilds_once_noop_and_invalid_order_do_not() {
    Fixture fixture;
    fixture.useThreshold();
    const char* topic =
        "envnode/Weather_Station/controller/1/cmd/parameter/on_threshold";
    const uint32_t before = fixture.runtime.compositionRevision();
    deliver(fixture.adapter, topic, "72");
    TEST_ASSERT_EQUAL_UINT32(1, fixture.configuration.controllerSetCount);
    TEST_ASSERT_EQUAL_UINT32(before + 1, fixture.runtime.compositionRevision());
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 72.0F, fixture.configuration.configuration
        .controllerSlots[0].implementationConfiguration.threshold.onThreshold);
    ControllerRuntimeInfo info;
    TEST_ASSERT_TRUE(fixture.runtime.runtimeInfo(0, info));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 72.0F, info.onThreshold);

    deliver(fixture.adapter, topic, "72");
    TEST_ASSERT_EQUAL_UINT32(1, fixture.configuration.controllerSetCount);
    TEST_ASSERT_EQUAL_UINT32(before + 1, fixture.runtime.compositionRevision());
    deliver(fixture.adapter, topic, "60");
    TEST_ASSERT_EQUAL_UINT32(2, fixture.configuration.controllerSetCount);
    TEST_ASSERT_EQUAL_UINT32(before + 1, fixture.runtime.compositionRevision());
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 72.0F, fixture.configuration.configuration
        .controllerSlots[0].implementationConfiguration.threshold.onThreshold);
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

void test_threshold_status_and_parameters_are_retained_and_unchanged_status_is_silent() {
    Fixture fixture;
    fixture.useThreshold();
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(4, fixture.mqtt.messages.size());
    const PublishedMessage* status = lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/status");
    TEST_ASSERT_NOT_NULL(status);
    TEST_ASSERT_EQUAL_STRING(
        "{\"running\":true,\"source_available\":false,\"measurement_valid\":false,\"stale\":false,\"decision\":\"unknown\",\"target_available\":false,\"output_pending\":false,\"last_result\":\"completed\"}",
        status->payload.c_str());
    TEST_ASSERT_TRUE(status->retained);
    const PublishedMessage* on = lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/parameter/on_threshold");
    const PublishedMessage* off = lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/parameter/off_threshold");
    const PublishedMessage* age = lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/parameter/max_measurement_age_ms");
    TEST_ASSERT_NOT_NULL(on);
    TEST_ASSERT_NOT_NULL(off);
    TEST_ASSERT_NOT_NULL(age);
    TEST_ASSERT_EQUAL_STRING("70", on->payload.c_str());
    TEST_ASSERT_EQUAL_STRING("65", off->payload.c_str());
    TEST_ASSERT_EQUAL_STRING("15000", age->payload.c_str());
    TEST_ASSERT_TRUE(on->retained);
    TEST_ASSERT_TRUE(off->retained);
    TEST_ASSERT_TRUE(age->retained);
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(4, fixture.mqtt.messages.size());
}

void test_threshold_status_tracks_decision_stale_target_and_stop_start() {
    Fixture fixture;
    fixture.useThreshold();
    fixture.publisher.loop();
    fixture.measurements.observe(temperatureMeasurement(72.0F), fixture.clock.now);
    fixture.runtime.loop();
    fixture.publisher.loop();
    const PublishedMessage* status = lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/status");
    TEST_ASSERT_TRUE(status->payload.find("\"source_available\":true") != std::string::npos);
    TEST_ASSERT_TRUE(status->payload.find("\"measurement_valid\":true") != std::string::npos);
    TEST_ASSERT_TRUE(status->payload.find("\"decision\":\"on\"") != std::string::npos);

    fixture.measurements.observe(temperatureMeasurement(60.0F), fixture.clock.now);
    fixture.runtime.loop();
    fixture.publisher.loop();
    status = lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/status");
    TEST_ASSERT_TRUE(status->payload.find("\"decision\":\"off\"") != std::string::npos);

    fixture.clock.now = 15001;
    fixture.runtime.loop();
    fixture.publisher.loop();
    status = lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/status");
    TEST_ASSERT_TRUE(status->payload.find("\"stale\":true") != std::string::npos);
    TEST_ASSERT_TRUE(status->payload.find("\"source_available\":false") != std::string::npos);
    TEST_ASSERT_TRUE(status->payload.find("\"decision\":\"off\"") != std::string::npos);

    fixture.resolver.target = nullptr;
    fixture.measurements.observe(temperatureMeasurement(72.0F), fixture.clock.now);
    fixture.runtime.loop();
    fixture.publisher.loop();
    status = lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/status");
    TEST_ASSERT_TRUE(status->payload.find("\"target_available\":false") != std::string::npos);
    TEST_ASSERT_TRUE(status->payload.find("\"output_pending\":true") != std::string::npos);

    const bool persistedEnabled =
        fixture.configuration.configuration.controllerSlots[0].enabled;
    deliver(fixture.adapter, "envnode/Weather_Station/controller/1/cmd", "STOP");
    fixture.publisher.loop();
    status = lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/status");
    TEST_ASSERT_TRUE(status->payload.find("\"running\":false") != std::string::npos);
    TEST_ASSERT_TRUE(status->payload.find("\"decision\":\"unknown\"") != std::string::npos);
    TEST_ASSERT_EQUAL(persistedEnabled,
        fixture.configuration.configuration.controllerSlots[0].enabled);
    fixture.resolver.target = &fixture.actuator;
    deliver(fixture.adapter, "envnode/Weather_Station/controller/1/cmd", "START");
    fixture.publisher.loop();
    status = lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/status");
    TEST_ASSERT_TRUE(status->payload.find("\"running\":true") != std::string::npos);
    TEST_ASSERT_EQUAL_UINT32(0, fixture.configuration.controllerSetCount);
}

void test_threshold_parameter_publication_tracks_mqtt_web_and_rejects_invalid_update() {
    Fixture fixture;
    fixture.useThreshold();
    fixture.publisher.loop();
    fixture.mqtt.messages.clear();
    deliver(fixture.adapter,
        "envnode/Weather_Station/controller/1/cmd/parameter/on_threshold", "72.5");
    fixture.publisher.loop();
    const PublishedMessage* on = lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/parameter/on_threshold");
    TEST_ASSERT_NOT_NULL(on);
    TEST_ASSERT_EQUAL_STRING("72.5", on->payload.c_str());

    fixture.mqtt.messages.clear();
    deliver(fixture.adapter,
        "envnode/Weather_Station/controller/1/cmd/parameter/on_threshold", "60");
    fixture.publisher.loop();
    TEST_ASSERT_NULL(lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/parameter/on_threshold"));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 72.5F, fixture.configuration.configuration
        .controllerSlots[0].implementationConfiguration.threshold.onThreshold);

    ControllerSlotConfiguration web =
        fixture.configuration.configuration.controllerSlots[0];
    web.implementationConfiguration.threshold.offThreshold = 64.25F;
    TEST_ASSERT_TRUE(fixture.configuration.setControllerSlotConfiguration(web));
    fixture.publisher.loop();
    const PublishedMessage* off = lastMessageForTopic(fixture.mqtt,
        "envnode/Weather_Station/controller/1/parameter/off_threshold");
    TEST_ASSERT_NOT_NULL(off);
    TEST_ASSERT_EQUAL_STRING("64.25", off->payload.c_str());
}

void test_threshold_reconnect_republishes_without_rebuild_or_feedback() {
    Fixture fixture;
    fixture.useThreshold();
    fixture.adapter.loop();
    fixture.publisher.loop();
    const uint32_t revision = fixture.runtime.compositionRevision();
    fixture.mqtt.isConnected = false;
    fixture.adapter.loop();
    fixture.publisher.loop();
    fixture.mqtt.isConnected = true;
    fixture.adapter.loop();
    fixture.publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(4, fixture.mqtt.subscriptions.size());
    TEST_ASSERT_EQUAL_UINT32(8, fixture.mqtt.messages.size());
    TEST_ASSERT_EQUAL_UINT32(0, fixture.configuration.controllerSetCount);
    TEST_ASSERT_EQUAL_UINT32(revision, fixture.runtime.compositionRevision());
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
    RUN_TEST(test_cross_implementation_parameters_are_rejected);
    RUN_TEST(test_start_stop_commands_are_transient_runtime_operations);
    RUN_TEST(test_invalid_commands_and_ids_are_rejected);
    RUN_TEST(test_parameter_update_persists_and_rebuilds_runtime);
    RUN_TEST(test_parameter_state_echo_and_unchanged_command_do_not_rebuild);
    RUN_TEST(test_invalid_parameter_payloads_do_not_mutate_configuration);
    RUN_TEST(test_threshold_float_parsing_accepts_protocol_decimals_and_rejects_invalid_text);
    RUN_TEST(test_threshold_age_parsing_and_configuration_validation_are_strict);
    RUN_TEST(test_threshold_parameter_update_rebuilds_once_noop_and_invalid_order_do_not);
    RUN_TEST(test_status_and_parameters_are_retained_and_change_detected);
    RUN_TEST(test_threshold_status_and_parameters_are_retained_and_unchanged_status_is_silent);
    RUN_TEST(test_threshold_status_tracks_decision_stale_target_and_stop_start);
    RUN_TEST(test_threshold_parameter_publication_tracks_mqtt_web_and_rejects_invalid_update);
    RUN_TEST(test_threshold_reconnect_republishes_without_rebuild_or_feedback);
    RUN_TEST(test_web_originated_parameter_change_is_published);
    RUN_TEST(test_reconnect_restores_subscriptions_and_republishes_state);
    RUN_TEST(test_runtime_rebuild_republishes_status_without_retaining_controller_pointer);
    RUN_TEST(test_router_delivers_to_both_independent_handlers);
    return UNITY_END();
}
