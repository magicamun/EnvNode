#include <unity.h>

#include <vector>

#include "ActuatorRuntime.h"
#include "BlinkController.h"
#include "ControllerFactory.h"
#include "ControllerRuntime.h"
#include "ActuatorStatePublisher.h"
#include "MeasurementSnapshotCache.h"
#include "ThresholdController.h"

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

class TestClock : public IMonotonicClock {
public:
    uint32_t now = 0;
    uint32_t nowMs() const override { return now; }
};

class TestActuator : public IOnOffActuator {
public:
    OnOffState current = OnOffState::Off;
    ActuatorOperationResult operationResult = ActuatorOperationResult::Completed;
    unsigned int setCount = 0;
    std::vector<OnOffState> commands;

    ActuatorOperationResult begin() override { return ActuatorOperationResult::Completed; }
    ActuatorOperationResult shutdown() override {
        current = OnOffState::Off;
        return ActuatorOperationResult::Completed;
    }
    ActuatorOperationResult setState(OnOffState state) override {
        ++setCount;
        commands.push_back(state);
        if (operationResult == ActuatorOperationResult::Completed) current = state;
        return operationResult;
    }
    OnOffState state() const override { return current; }
    bool initialized() const override { return true; }
};

class TestResolver : public IOnOffActuatorResolver {
public:
    IOnOffActuator* targets[MaxActuatorSlotCount] = {};
    IOnOffActuator* onOffActuator(ActuatorId id) override {
        return isValidActuatorId(id) && id <= MaxActuatorSlotCount
            ? targets[id - 1] : nullptr;
    }
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

BlinkControllerConfiguration blinkConfiguration(
    ActuatorId target = 1,
    uint32_t onMs = 100,
    uint32_t offMs = 200) {
    BlinkControllerConfiguration configuration;
    configuration.targetActuatorId = target;
    configuration.onDurationMs = onMs;
    configuration.offDurationMs = offMs;
    return configuration;
}

void initializeControllerSlots(ControllerSlotConfiguration* slots) {
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        slots[index].slotId = static_cast<ControllerId>(index + 1);
        slots[index].enabled = false;
        slots[index].name = "Unused";
        slots[index].implementation = ControllerImplementation::None;
    }
}

void configureBlinkSlot(
    ControllerSlotConfiguration& slot,
    ActuatorId target,
    uint32_t onMs = 100,
    uint32_t offMs = 200) {
    slot.enabled = true;
    slot.name = "Blink";
    slot.implementation = ControllerImplementation::Blink;
    slot.implementationConfiguration.blink = blinkConfiguration(target, onMs, offMs);
}

void initializeActuatorSlots(ActuatorSlotConfiguration* slots) {
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        slots[index].slotId = static_cast<ActuatorId>(index + 1);
        slots[index].enabled = false;
        slots[index].name = "Unused";
        slots[index].implementation = ActuatorImplementation::None;
        slots[index].hardware = HardwareResourceAssignment::none();
    }
}

void configureActuatorSlot(ActuatorSlotConfiguration& slot, uint8_t gpio) {
    slot.enabled = true;
    slot.name = "Target";
    slot.implementation = ActuatorImplementation::GpioOnOff;
    slot.hardware = HardwareResourceAssignment::gpioResource(GpioResource(gpio));
}

void test_blink_begin_and_deadline_transitions_are_non_blocking() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestResolver resolver;
    resolver.targets[0] = &actuator;
    BlinkController controller(blinkConfiguration(), resolver, clock, logger);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(controller.begin()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On), static_cast<int>(actuator.state()));
    clock.now = 99;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::NoAction),
        static_cast<int>(controller.service()));
    TEST_ASSERT_EQUAL_UINT32(1, actuator.setCount);
    clock.now = 100;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(controller.service()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off), static_cast<int>(actuator.state()));
    clock.now = 299;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::NoAction),
        static_cast<int>(controller.service()));
    clock.now = 300;
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On), static_cast<int>(actuator.state()));
}

void test_blink_deadline_is_wrap_safe() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestResolver resolver;
    resolver.targets[0] = &actuator;
    clock.now = 0xFFFFFFF0UL;
    BlinkController controller(blinkConfiguration(1, 32, 20), resolver, clock, logger);
    controller.begin();
    clock.now = 15;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::NoAction),
        static_cast<int>(controller.service()));
    clock.now = 16;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(controller.service()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off), static_cast<int>(actuator.state()));
}

void test_unavailable_target_restarts_fresh_cycle_when_available() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestResolver resolver;
    BlinkController controller(blinkConfiguration(), resolver, clock, logger);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::TargetUnavailable),
        static_cast<int>(controller.begin()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BlinkPhase::WaitingForTarget),
        static_cast<int>(controller.phase()));
    resolver.targets[0] = &actuator;
    clock.now = 50;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(controller.service()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BlinkPhase::On), static_cast<int>(controller.phase()));
    TEST_ASSERT_EQUAL_UINT32(150, controller.nextTransitionMs());

    clock.now = 150;
    resolver.targets[0] = nullptr;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::TargetUnavailable),
        static_cast<int>(controller.service()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BlinkPhase::WaitingForTarget),
        static_cast<int>(controller.phase()));
    resolver.targets[0] = &actuator;
    clock.now = 170;
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On), static_cast<int>(actuator.state()));
    TEST_ASSERT_EQUAL_UINT32(270, controller.nextTransitionMs());
}

void test_stop_commands_off_and_manual_change_waits_for_deadline() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestResolver resolver;
    resolver.targets[0] = &actuator;
    BlinkController controller(blinkConfiguration(), resolver, clock, logger);
    controller.begin();
    actuator.setState(OnOffState::Off);
    const unsigned int afterManualCommand = actuator.setCount;
    clock.now = 99;
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(afterManualCommand, actuator.setCount);
    actuator.setState(OnOffState::On);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(controller.stop()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off), static_cast<int>(actuator.state()));
}

void test_controller_resolves_replacement_instead_of_retaining_pointer() {
    TestLogger logger;
    TestClock clock;
    TestActuator first;
    TestActuator replacement;
    TestResolver resolver;
    resolver.targets[0] = &first;
    BlinkController controller(blinkConfiguration(), resolver, clock, logger);
    controller.begin();
    resolver.targets[0] = &replacement;
    clock.now = 100;
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(1, first.setCount);
    TEST_ASSERT_EQUAL_UINT32(1, replacement.setCount);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off),
        static_cast<int>(replacement.state()));
}

void test_factory_and_runtime_handle_slots_and_multiple_controllers() {
    TestLogger logger;
    TestClock clock;
    TestActuator first;
    TestActuator second;
    TestResolver resolver;
    resolver.targets[0] = &first;
    resolver.targets[1] = &second;
    MeasurementSnapshotCache measurements;
    ControllerFactory factory(measurements, resolver, clock, logger);
    ControllerRuntime runtime(factory, logger);
    ControllerSlotConfiguration slots[MaxControllerSlotCount];
    initializeControllerSlots(slots);
    configureBlinkSlot(slots[0], 1);
    configureBlinkSlot(slots[1], 2);

    TEST_ASSERT_TRUE(runtime.initialize(slots));
    TEST_ASSERT_EQUAL_UINT32(2, runtime.runtimeCount());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On), static_cast<int>(first.state()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On), static_cast<int>(second.state()));
    clock.now = 100;
    runtime.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off), static_cast<int>(first.state()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off), static_cast<int>(second.state()));

    ControllerRuntimeInfo info;
    TEST_ASSERT_TRUE(runtime.runtimeInfo(0, info));
    TEST_ASSERT_TRUE(info.running);
    TEST_ASSERT_TRUE(info.targetAvailable);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BlinkPhase::Off), static_cast<int>(info.blinkPhase));
}

void test_disabled_and_none_slots_create_no_runtime_controller() {
    TestLogger logger;
    TestClock clock;
    TestResolver resolver;
    MeasurementSnapshotCache measurements;
    ControllerFactory factory(measurements, resolver, clock, logger);
    ControllerRuntime runtime(factory, logger);
    ControllerSlotConfiguration slots[MaxControllerSlotCount];
    initializeControllerSlots(slots);
    slots[0].enabled = true;
    TEST_ASSERT_TRUE(runtime.initialize(slots));
    TEST_ASSERT_EQUAL_UINT32(0, runtime.runtimeCount());
}

void test_runtime_rebuild_stops_old_and_activates_new_composition() {
    TestLogger logger;
    TestClock clock;
    TestActuator first;
    TestActuator second;
    TestResolver resolver;
    resolver.targets[0] = &first;
    resolver.targets[1] = &second;
    MeasurementSnapshotCache measurements;
    ControllerFactory factory(measurements, resolver, clock, logger);
    ControllerRuntime runtime(factory, logger);
    ControllerSlotConfiguration slots[MaxControllerSlotCount];
    initializeControllerSlots(slots);
    configureBlinkSlot(slots[0], 1);
    TEST_ASSERT_TRUE(runtime.initialize(slots));

    slots[0].enabled = false;
    configureBlinkSlot(slots[1], 2);
    TEST_ASSERT_TRUE(runtime.rebuild(slots));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off), static_cast<int>(first.state()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On), static_cast<int>(second.state()));
    TEST_ASSERT_EQUAL_UINT32(1, runtime.runtimeCount());
}

void test_runtime_start_and_stop_are_transient_and_restart_a_fresh_cycle() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestResolver resolver;
    resolver.targets[0] = &actuator;
    MeasurementSnapshotCache measurements;
    ControllerFactory factory(measurements, resolver, clock, logger);
    ControllerRuntime runtime(factory, logger);
    ControllerSlotConfiguration slots[MaxControllerSlotCount];
    initializeControllerSlots(slots);
    configureBlinkSlot(slots[0], 1, 100, 200);
    TEST_ASSERT_TRUE(runtime.initialize(slots));

    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(runtime.stopController(1)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off),
        static_cast<int>(actuator.state()));
    ControllerRuntimeInfo info;
    TEST_ASSERT_TRUE(runtime.runtimeInfo(0, info));
    TEST_ASSERT_FALSE(info.running);
    TEST_ASSERT_TRUE(slots[0].enabled);

    clock.now = 50;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(runtime.startController(1)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On),
        static_cast<int>(actuator.state()));
    clock.now = 149;
    runtime.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On),
        static_cast<int>(actuator.state()));
    clock.now = 150;
    runtime.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off),
        static_cast<int>(actuator.state()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::ControllerNotFound),
        static_cast<int>(runtime.startController(2)));
}

void test_rebuild_applies_updated_timing_and_restarts_stopped_controller() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestResolver resolver;
    resolver.targets[0] = &actuator;
    MeasurementSnapshotCache measurements;
    ControllerFactory factory(measurements, resolver, clock, logger);
    ControllerRuntime runtime(factory, logger);
    ControllerSlotConfiguration slots[MaxControllerSlotCount];
    initializeControllerSlots(slots);
    configureBlinkSlot(slots[0], 1, 100, 200);
    TEST_ASSERT_TRUE(runtime.initialize(slots));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(runtime.stopController(1)));

    slots[0].implementationConfiguration.blink.onDurationMs = 250;
    slots[0].implementationConfiguration.blink.offDurationMs = 750;
    clock.now = 20;
    TEST_ASSERT_TRUE(runtime.rebuild(slots));
    ControllerRuntimeInfo info;
    TEST_ASSERT_TRUE(runtime.runtimeInfo(0, info));
    TEST_ASSERT_TRUE(info.running);
    TEST_ASSERT_EQUAL_UINT32(250, info.onDurationMs);
    TEST_ASSERT_EQUAL_UINT32(750, info.offDurationMs);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On),
        static_cast<int>(actuator.state()));
    clock.now = 269;
    runtime.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On),
        static_cast<int>(actuator.state()));
    clock.now = 270;
    runtime.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off),
        static_cast<int>(actuator.state()));
}

void test_failed_rebuild_leaves_active_composition_untouched() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestResolver resolver;
    resolver.targets[0] = &actuator;
    MeasurementSnapshotCache measurements;
    ControllerFactory factory(measurements, resolver, clock, logger);
    ControllerRuntime runtime(factory, logger);
    ControllerSlotConfiguration slots[MaxControllerSlotCount];
    initializeControllerSlots(slots);
    configureBlinkSlot(slots[0], 1);
    TEST_ASSERT_TRUE(runtime.initialize(slots));
    const unsigned int before = actuator.setCount;
    slots[0].implementationConfiguration.blink.onDurationMs = 0;
    TEST_ASSERT_FALSE(runtime.rebuild(slots));
    TEST_ASSERT_EQUAL_UINT32(before, actuator.setCount);
    TEST_ASSERT_EQUAL_UINT32(1, runtime.runtimeCount());
}

void test_actuator_runtime_replacement_is_resolved_by_controller() {
    TestLogger logger;
    TestClock clock;
    ActuatorFactory actuatorFactory(logger);
    ActuatorRuntime actuatorRuntime(actuatorFactory, logger);
    ActuatorSlotConfiguration actuatorSlots[MaxActuatorSlotCount];
    initializeActuatorSlots(actuatorSlots);
    configureActuatorSlot(actuatorSlots[0], 16);
    actuatorRuntime.initialize(actuatorSlots);
    TEST_ASSERT_NOT_NULL(actuatorRuntime.onOffActuator(1));

    MeasurementSnapshotCache measurements;
    ControllerFactory controllerFactory(
        measurements, actuatorRuntime, clock, logger);
    ControllerRuntime controllerRuntime(controllerFactory, logger);
    ControllerSlotConfiguration controllerSlots[MaxControllerSlotCount];
    initializeControllerSlots(controllerSlots);
    configureBlinkSlot(controllerSlots[0], 1);
    TEST_ASSERT_TRUE(controllerRuntime.initialize(controllerSlots));
    TEST_ASSERT_EQUAL_INT(HIGH, gpioValues[16]);

    actuatorSlots[0].hardware =
        HardwareResourceAssignment::gpioResource(GpioResource(17));
    TEST_ASSERT_TRUE(actuatorRuntime.rebuild(actuatorSlots));
    TEST_ASSERT_EQUAL_INT(LOW, gpioValues[16]);
    clock.now = 100;
    controllerRuntime.loop();
    TEST_ASSERT_EQUAL_INT(LOW, gpioValues[17]);
    clock.now = 300;
    controllerRuntime.loop();
    TEST_ASSERT_EQUAL_INT(HIGH, gpioValues[17]);
}

void test_mqtt_state_publisher_observes_blink_driven_changes() {
    TestLogger logger;
    TestClock clock;
    ActuatorFactory actuatorFactory(logger);
    ActuatorRuntime actuatorRuntime(actuatorFactory, logger);
    ActuatorSlotConfiguration actuatorSlots[MaxActuatorSlotCount];
    initializeActuatorSlots(actuatorSlots);
    configureActuatorSlot(actuatorSlots[0], 16);
    actuatorRuntime.initialize(actuatorSlots);

    MeasurementSnapshotCache measurements;
    ControllerFactory controllerFactory(
        measurements, actuatorRuntime, clock, logger);
    ControllerRuntime controllerRuntime(controllerFactory, logger);
    ControllerSlotConfiguration controllerSlots[MaxControllerSlotCount];
    initializeControllerSlots(controllerSlots);
    configureBlinkSlot(controllerSlots[0], 1);
    TEST_ASSERT_TRUE(controllerRuntime.initialize(controllerSlots));

    TestConfigurationService configuration;
    configuration.configuration.device.name = "ControllerTest";
    TestMqttService mqtt;
    ActuatorStatePublisher publisher(
        logger, configuration, mqtt, actuatorRuntime);
    publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(1, mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("ON", mqtt.messages[0].payload.c_str());
    TEST_ASSERT_TRUE(mqtt.messages[0].retained);

    clock.now = 100;
    controllerRuntime.loop();
    publisher.loop();
    TEST_ASSERT_EQUAL_UINT32(2, mqtt.messages.size());
    TEST_ASSERT_EQUAL_STRING("OFF", mqtt.messages[1].payload.c_str());
    TEST_ASSERT_TRUE(mqtt.messages[1].retained);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_blink_begin_and_deadline_transitions_are_non_blocking);
    RUN_TEST(test_blink_deadline_is_wrap_safe);
    RUN_TEST(test_unavailable_target_restarts_fresh_cycle_when_available);
    RUN_TEST(test_stop_commands_off_and_manual_change_waits_for_deadline);
    RUN_TEST(test_controller_resolves_replacement_instead_of_retaining_pointer);
    RUN_TEST(test_factory_and_runtime_handle_slots_and_multiple_controllers);
    RUN_TEST(test_disabled_and_none_slots_create_no_runtime_controller);
    RUN_TEST(test_runtime_rebuild_stops_old_and_activates_new_composition);
    RUN_TEST(test_runtime_start_and_stop_are_transient_and_restart_a_fresh_cycle);
    RUN_TEST(test_rebuild_applies_updated_timing_and_restarts_stopped_controller);
    RUN_TEST(test_failed_rebuild_leaves_active_composition_untouched);
    RUN_TEST(test_actuator_runtime_replacement_is_resolved_by_controller);
    RUN_TEST(test_mqtt_state_publisher_observes_blink_driven_changes);
    return UNITY_END();
}
