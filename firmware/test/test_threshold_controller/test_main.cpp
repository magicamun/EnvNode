#include <unity.h>

#include <vector>

#include "ActuatorSlotConfiguration.h"
#include "ActuatorRuntime.h"
#include "ControllerFactory.h"
#include "ControllerRuntime.h"
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
    ActuatorOperationResult shutdown() override { return ActuatorOperationResult::Completed; }
    ActuatorOperationResult setState(OnOffState state) override {
        ++setCount;
        commands.push_back(state);
        if (operationResult == ActuatorOperationResult::Completed) current = state;
        return operationResult;
    }
    OnOffState state() const override { return current; }
    bool initialized() const override { return true; }
};

class TestActuatorResolver : public IOnOffActuatorResolver {
public:
    IOnOffActuator* targets[MaxActuatorSlotCount] = {};
    IOnOffActuator* onOffActuator(ActuatorId id) override {
        return isValidActuatorId(id) && id <= MaxActuatorSlotCount
            ? targets[id - 1] : nullptr;
    }
};

class TestMeasurementResolver : public IMeasurementResolver {
public:
    bool available = false;
    MeasurementSnapshot current;
    unsigned int lookupCount = 0;

    bool latest(const MeasurementSourceReference&, MeasurementSnapshot& result) const override {
        ++const_cast<TestMeasurementResolver*>(this)->lookupCount;
        if (!available) return false;
        result = current;
        return true;
    }
};

ThresholdControllerConfiguration thresholdConfiguration() {
    ThresholdControllerConfiguration configuration;
    configuration.source = MeasurementSourceReference(1, MeasurementType::RelativeHumidity);
    configuration.targetActuatorId = 1;
    configuration.onThreshold = 70.0F;
    configuration.offThreshold = 65.0F;
    configuration.maxMeasurementAgeMs = 1000;
    return configuration;
}

MeasurementSnapshot snapshot(
    float value,
    uint32_t acceptedMs,
    uint32_t revision,
    bool valid = true,
    SensorId source = 1,
    MeasurementType type = MeasurementType::RelativeHumidity) {
    MeasurementSnapshot result;
    result.measurement.source = source;
    result.measurement.type = type;
    result.measurement.timestamp = 1;
    result.measurement.valid = valid;
    result.measurement.quality = MeasurementQuality::Good;
    result.measurement.value = valid
        ? MeasurementValue::floatingPoint(value) : MeasurementValue::none();
    result.acceptedMonotonicMs = acceptedMs;
    result.revision = revision;
    return result;
}

void setSnapshot(
    TestMeasurementResolver& resolver,
    float value,
    uint32_t acceptedMs,
    uint32_t revision) {
    resolver.available = true;
    resolver.current = snapshot(value, acceptedMs, revision);
}

void initializeControllerSlots(ControllerSlotConfiguration* slots) {
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        slots[index].slotId = static_cast<ControllerId>(index + 1);
        slots[index].enabled = false;
        slots[index].name = "Unused";
        slots[index].implementation = ControllerImplementation::None;
    }
}

void configureThresholdSlot(
    ControllerSlotConfiguration& slot,
    const ThresholdControllerConfiguration& configuration = thresholdConfiguration()) {
    slot.enabled = true;
    slot.name = "Threshold";
    slot.implementation = ControllerImplementation::Threshold;
    slot.implementationConfiguration.threshold = configuration;
}

void configureBlinkSlot(ControllerSlotConfiguration& slot, ActuatorId target) {
    slot.enabled = true;
    slot.name = "Blink";
    slot.implementation = ControllerImplementation::Blink;
    slot.implementationConfiguration.blink.targetActuatorId = target;
    slot.implementationConfiguration.blink.onDurationMs = 100;
    slot.implementationConfiguration.blink.offDurationMs = 200;
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

void test_begin_handles_missing_stale_invalid_and_initial_in_band_input() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    TestMeasurementResolver measurements;

    ThresholdController missing(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(missing.begin()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::Unknown),
        static_cast<int>(missing.decision()));
    TEST_ASSERT_EQUAL_UINT32(0, actuator.setCount);

    clock.now = 2000;
    measurements.available = true;
    measurements.current = snapshot(72.0F, 999, 1);
    ThresholdController stale(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(stale.begin()));
    TEST_ASSERT_TRUE(stale.latestSnapshotStale());
    TEST_ASSERT_FALSE(stale.sourceAvailable());

    measurements.current = snapshot(72.0F, 2000, 2, false);
    ThresholdController invalid(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    invalid.begin();
    TEST_ASSERT_FALSE(invalid.sourceAvailable());
    TEST_ASSERT_TRUE(invalid.hasProcessedRevision());

    measurements.current = snapshot(68.0F, 2000, 3);
    ThresholdController inBand(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    inBand.begin();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::Unknown),
        static_cast<int>(inBand.decision()));
    TEST_ASSERT_EQUAL_UINT32(0, actuator.setCount);
}

void test_begin_commands_on_or_off_at_thresholds_and_accepts_quality_labels() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    TestMeasurementResolver measurements;
    setSnapshot(measurements, 70.0F, 0, 1);
    measurements.current.measurement.quality = MeasurementQuality::Degraded;
    ThresholdController on(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(on.begin()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::On),
        static_cast<int>(on.decision()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On),
        static_cast<int>(actuator.current));

    actuator.setCount = 0;
    measurements.current = snapshot(65.0F, 0, 2);
    measurements.current.measurement.quality = MeasurementQuality::Estimated;
    ThresholdController off(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    off.begin();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::Off),
        static_cast<int>(off.decision()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off),
        static_cast<int>(actuator.current));
    TEST_ASSERT_EQUAL_UINT32(1, actuator.setCount);
}

void test_hysteresis_processes_revisions_without_repeating_commands() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    TestMeasurementResolver measurements;
    setSnapshot(measurements, 72.0F, 0, 1);
    ThresholdController controller(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    controller.begin();

    const float values[] = {68.0F, 66.0F, 64.0F, 67.0F, 71.0F};
    for (uint32_t index = 0; index < 5; ++index) {
        setSnapshot(measurements, values[index], 0, index + 2);
        controller.service();
    }
    TEST_ASSERT_EQUAL_UINT32(3, actuator.setCount);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On),
        static_cast<int>(actuator.commands[0]));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off),
        static_cast<int>(actuator.commands[1]));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On),
        static_cast<int>(actuator.commands[2]));

    controller.service();
    TEST_ASSERT_EQUAL_UINT32(3, actuator.setCount);
    setSnapshot(measurements, 71.0F, 0, 7);
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(3, actuator.setCount);
    TEST_ASSERT_EQUAL_UINT32(7, controller.lastProcessedRevision());
}

void test_freshness_boundary_wraparound_and_stale_input_do_not_force_off() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    TestMeasurementResolver measurements;
    setSnapshot(measurements, 72.0F, 100, 1);
    clock.now = 1100;
    ThresholdController controller(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    controller.begin();
    TEST_ASSERT_TRUE(controller.sourceAvailable());
    TEST_ASSERT_EQUAL_UINT32(1, actuator.setCount);

    clock.now = 1101;
    controller.service();
    TEST_ASSERT_TRUE(controller.latestSnapshotStale());
    TEST_ASSERT_FALSE(controller.sourceAvailable());
    TEST_ASSERT_EQUAL_UINT32(1, actuator.setCount);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::On),
        static_cast<int>(controller.decision()));

    setSnapshot(measurements, 64.0F, 1101, 2);
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::Off),
        static_cast<int>(controller.decision()));
    TEST_ASSERT_EQUAL_UINT32(2, actuator.setCount);

    TestActuator wrapActuator;
    actuators.targets[0] = &wrapActuator;
    setSnapshot(measurements, 72.0F, 0xFFFFFFF0UL, 3);
    clock.now = 20;
    ThresholdController wrapped(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    wrapped.begin();
    TEST_ASSERT_EQUAL_UINT32(36, wrapped.latestSnapshotAgeMs());
    TEST_ASSERT_TRUE(wrapped.sourceAvailable());
}

void test_incompatible_and_invalid_snapshots_are_processed_without_decision() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    TestMeasurementResolver measurements;
    ThresholdController controller(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    controller.begin();

    measurements.available = true;
    measurements.current = snapshot(72.0F, 0, 1, true, 2);
    controller.service();
    TEST_ASSERT_FALSE(controller.sourceAvailable());
    TEST_ASSERT_EQUAL_UINT32(1, controller.lastProcessedRevision());
    measurements.current = snapshot(72.0F, 0, 2, true, 1, MeasurementType::Temperature);
    controller.service();
    measurements.current = snapshot(72.0F, 0, 3);
    measurements.current.measurement.value = MeasurementValue::boolean(true);
    controller.service();
    measurements.current = snapshot(72.0F, 0, 4, false);
    controller.service();
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(4, controller.lastProcessedRevision());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::Unknown),
        static_cast<int>(controller.decision()));
    TEST_ASSERT_EQUAL_UINT32(0, actuator.setCount);
}

void test_pending_output_retries_without_measurement_and_resolves_replacement() {
    TestLogger logger;
    TestClock clock;
    TestActuator first;
    TestActuator replacement;
    TestActuatorResolver actuators;
    TestMeasurementResolver measurements;
    setSnapshot(measurements, 72.0F, 0, 1);
    ThresholdController controller(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::TargetUnavailable),
        static_cast<int>(controller.begin()));
    TEST_ASSERT_TRUE(controller.outputApplicationPending());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::On),
        static_cast<int>(controller.decision()));

    actuators.targets[0] = &first;
    first.operationResult = ActuatorOperationResult::NotInitialized;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::ActuatorOperationFailed),
        static_cast<int>(controller.service()));
    TEST_ASSERT_TRUE(controller.outputApplicationPending());
    actuators.targets[0] = &replacement;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(controller.service()));
    TEST_ASSERT_FALSE(controller.outputApplicationPending());
    TEST_ASSERT_EQUAL_UINT32(1, first.setCount);
    TEST_ASSERT_EQUAL_UINT32(1, replacement.setCount);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On),
        static_cast<int>(replacement.current));
}

void test_manual_contention_is_not_reconciled_until_decision_changes() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    TestMeasurementResolver measurements;
    setSnapshot(measurements, 72.0F, 0, 1);
    ThresholdController controller(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    controller.begin();
    actuator.setState(OnOffState::Off);
    const unsigned int afterManual = actuator.setCount;
    controller.service();
    setSnapshot(measurements, 68.0F, 0, 2);
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(afterManual, actuator.setCount);
    setSnapshot(measurements, 64.0F, 0, 3);
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(afterManual + 1, actuator.setCount);
    setSnapshot(measurements, 71.0F, 0, 4);
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(afterManual + 2, actuator.setCount);
}

void test_cache_clear_removes_source_without_erasing_decision() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    MeasurementSnapshotCache cache;
    Measurement first = snapshot(72.0F, 0, 0).measurement;
    cache.observe(first, 0);
    ThresholdController controller(
        thresholdConfiguration(), cache, actuators, clock, logger);
    controller.begin();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::On),
        static_cast<int>(controller.decision()));
    cache.clear();
    controller.service();
    TEST_ASSERT_FALSE(controller.sourceAvailable());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::On),
        static_cast<int>(controller.decision()));
    TEST_ASSERT_EQUAL_UINT32(1, actuator.setCount);

    Measurement next = snapshot(64.0F, 0, 0).measurement;
    cache.observe(next, 0);
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::Off),
        static_cast<int>(controller.decision()));
    TEST_ASSERT_EQUAL_UINT32(2, actuator.setCount);
}

void test_stop_and_start_reset_decision_and_evaluate_current_snapshot() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    TestMeasurementResolver measurements;
    setSnapshot(measurements, 72.0F, 0, 1);
    ThresholdController controller(
        thresholdConfiguration(), measurements, actuators, clock, logger);
    controller.begin();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(controller.stop()));
    TEST_ASSERT_FALSE(controller.running());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off),
        static_cast<int>(actuator.current));
    const unsigned int stoppedCount = actuator.setCount;
    setSnapshot(measurements, 64.0F, 0, 2);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::NotRunning),
        static_cast<int>(controller.service()));
    TEST_ASSERT_EQUAL_UINT32(stoppedCount, actuator.setCount);
    controller.begin();
    TEST_ASSERT_TRUE(controller.running());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::Off),
        static_cast<int>(controller.decision()));
    TEST_ASSERT_EQUAL_UINT32(stoppedCount + 1, actuator.setCount);
}

void test_factory_and_runtime_support_mixed_composition_and_live_rebuild() {
    TestLogger logger;
    TestClock clock;
    TestActuator blinkActuator;
    TestActuator thresholdActuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &blinkActuator;
    actuators.targets[1] = &thresholdActuator;
    MeasurementSnapshotCache measurements;
    Measurement humidity = snapshot(72.0F, 0, 0).measurement;
    measurements.observe(humidity, 0);
    ControllerFactory factory(measurements, actuators, clock, logger);
    ControllerRuntime runtime(factory, logger);
    ControllerSlotConfiguration slots[MaxControllerSlotCount];
    initializeControllerSlots(slots);
    configureBlinkSlot(slots[0], 1);
    ThresholdControllerConfiguration threshold = thresholdConfiguration();
    threshold.targetActuatorId = 2;
    configureThresholdSlot(slots[1], threshold);
    TEST_ASSERT_TRUE(runtime.initialize(slots));
    TEST_ASSERT_EQUAL_UINT32(2, runtime.runtimeCount());
    ControllerRuntimeInfo info;
    TEST_ASSERT_TRUE(runtime.runtimeInfo(1, info));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerImplementation::Threshold),
        static_cast<int>(info.implementation));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::On),
        static_cast<int>(info.thresholdDecision));
    const unsigned int thresholdCommands = thresholdActuator.setCount;
    clock.now = 100;
    runtime.loop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off),
        static_cast<int>(blinkActuator.current));
    TEST_ASSERT_EQUAL_UINT32(thresholdCommands, thresholdActuator.setCount);

    slots[1].implementationConfiguration.threshold.onThreshold = 80.0F;
    slots[1].implementationConfiguration.threshold.offThreshold = 75.0F;
    TEST_ASSERT_TRUE(runtime.rebuild(slots));
    TEST_ASSERT_TRUE(runtime.runtimeInfo(1, info));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::Off),
        static_cast<int>(info.thresholdDecision));
    TEST_ASSERT_EQUAL_UINT32(2, runtime.runtimeCount());
}

void test_actuator_runtime_replacement_is_resolved_without_reasserting_output() {
    TestLogger logger;
    TestClock clock;
    ActuatorFactory actuatorFactory(logger);
    ActuatorRuntime actuatorRuntime(actuatorFactory, logger);
    ActuatorSlotConfiguration actuatorSlots[MaxActuatorSlotCount];
    initializeActuatorSlots(actuatorSlots);
    actuatorSlots[0].enabled = true;
    actuatorSlots[0].name = "Target";
    actuatorSlots[0].implementation = ActuatorImplementation::GpioOnOff;
    actuatorSlots[0].hardware =
        HardwareResourceAssignment::gpioResource(GpioResource(16));
    actuatorRuntime.initialize(actuatorSlots);
    TEST_ASSERT_NOT_NULL(actuatorRuntime.onOffActuator(1));

    MeasurementSnapshotCache measurements;
    Measurement humidity = snapshot(72.0F, 0, 0).measurement;
    measurements.observe(humidity, 0);
    ThresholdController controller(
        thresholdConfiguration(), measurements, actuatorRuntime, clock, logger);
    controller.begin();
    TEST_ASSERT_EQUAL_INT(HIGH, gpioValues[16]);

    actuatorSlots[0].hardware =
        HardwareResourceAssignment::gpioResource(GpioResource(17));
    TEST_ASSERT_TRUE(actuatorRuntime.rebuild(actuatorSlots));
    TEST_ASSERT_EQUAL_INT(LOW, gpioValues[16]);
    TEST_ASSERT_EQUAL_INT(LOW, gpioValues[17]);
    controller.service();
    TEST_ASSERT_EQUAL_INT(LOW, gpioValues[17]);

    humidity = snapshot(64.0F, 0, 0).measurement;
    measurements.observe(humidity, 0);
    controller.service();
    humidity = snapshot(72.0F, 0, 0).measurement;
    measurements.observe(humidity, 0);
    controller.service();
    TEST_ASSERT_EQUAL_INT(HIGH, gpioValues[17]);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_begin_handles_missing_stale_invalid_and_initial_in_band_input);
    RUN_TEST(test_begin_commands_on_or_off_at_thresholds_and_accepts_quality_labels);
    RUN_TEST(test_hysteresis_processes_revisions_without_repeating_commands);
    RUN_TEST(test_freshness_boundary_wraparound_and_stale_input_do_not_force_off);
    RUN_TEST(test_incompatible_and_invalid_snapshots_are_processed_without_decision);
    RUN_TEST(test_pending_output_retries_without_measurement_and_resolves_replacement);
    RUN_TEST(test_manual_contention_is_not_reconciled_until_decision_changes);
    RUN_TEST(test_cache_clear_removes_source_without_erasing_decision);
    RUN_TEST(test_stop_and_start_reset_decision_and_evaluate_current_snapshot);
    RUN_TEST(test_factory_and_runtime_support_mixed_composition_and_live_rebuild);
    RUN_TEST(test_actuator_runtime_replacement_is_resolved_without_reasserting_output);
    return UNITY_END();
}
