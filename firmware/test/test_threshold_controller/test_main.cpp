#include <unity.h>

#include <cstring>
#include <string>
#include <vector>

#include "ActuatorSlotConfiguration.h"
#include "ActuatorRuntime.h"
#include "ControllerFactory.h"
#include "ControllerRuntime.h"
#include "MeasurementSnapshotCache.h"
#include "ThresholdController.h"
#include "ControllerPropertyReader.h"
#include "PropertyWebView.h"

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
    struct Entry {
        LogLevel level;
        std::string message;
    };

    explicit TestLogger(LogLevel minimum = LogLevel::Info) : minimum_(minimum) {}
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
    bool accepts(LogLevel level) const override {
        return static_cast<uint8_t>(level) >= static_cast<uint8_t>(minimum_);
    }
    void log(LogLevel level, const char* message) override {
        entries.push_back({level, message == nullptr ? "" : message});
    }
    size_t count(LogLevel level) const {
        size_t result = 0;
        for (const Entry& entry : entries) if (entry.level == level) ++result;
        return result;
    }
    const Entry* find(LogLevel level, const char* text) const {
        for (const Entry& entry : entries) {
            if (entry.level == level && entry.message.find(text) != std::string::npos) {
                return &entry;
            }
        }
        return nullptr;
    }

    std::vector<Entry> entries;

private:
    LogLevel minimum_;
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

void test_inverse_hysteresis_switches_on_below_and_off_above() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    TestMeasurementResolver measurements;
    ThresholdControllerConfiguration configuration = thresholdConfiguration();
    configuration.direction = ThresholdDirection::OnBelow;
    configuration.onThreshold = 20.0F;
    configuration.offThreshold = 25.0F;
    setSnapshot(measurements, 20.0F, 0, 1);
    ThresholdController controller(
        configuration, measurements, actuators, clock, logger);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerOperationResult::Completed),
        static_cast<int>(controller.begin()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On),
        static_cast<int>(actuator.current));
    setSnapshot(measurements, 22.0F, 0, 2);
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(1, actuator.setCount);
    setSnapshot(measurements, 25.0F, 0, 3);
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off),
        static_cast<int>(actuator.current));
    TEST_ASSERT_EQUAL_UINT32(2, actuator.setCount);
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
    TEST_ASSERT_NULL(runtime.reasonProvider(1));
    TEST_ASSERT_NULL(runtime.reasonProvider(0));
    TEST_ASSERT_NOT_NULL(runtime.reasonProvider(2));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::OnThreshold),
        static_cast<int>(runtime.reasonProvider(2)->reason()));
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
    TEST_ASSERT_NOT_NULL(runtime.reasonProvider(2));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::OffThreshold),
        static_cast<int>(runtime.reasonProvider(2)->reason()));
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

void test_threshold_debug_evaluation_and_info_transitions_are_self_contained() {
    TestLogger logger(LogLevel::Debug);
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    TestMeasurementResolver measurements;
    setSnapshot(measurements, 72.0F, 100, 1);
    clock.now = 150;
    ThresholdController controller(
        thresholdConfiguration(), measurements, actuators, clock, logger,
        3, "Heater Control");
    controller.begin();

    const TestLogger::Entry* evaluation = logger.find(LogLevel::Debug, "evaluate Sensor 1");
    TEST_ASSERT_NOT_NULL(evaluation);
    TEST_ASSERT_NOT_NULL(strstr(evaluation->message.c_str(), "Controller 3 \"Heater Control\""));
    TEST_ASSERT_NOT_NULL(strstr(evaluation->message.c_str(), "relative_humidity=72 %"));
    TEST_ASSERT_NOT_NULL(strstr(evaluation->message.c_str(), "thresholds off=65 on=70 %"));
    TEST_ASSERT_NOT_NULL(strstr(evaluation->message.c_str(), "age=50 ms/1000 ms"));
    TEST_ASSERT_NOT_NULL(strstr(evaluation->message.c_str(), "decision=Unknown -> On"));

    const TestLogger::Entry* on = logger.find(LogLevel::Info, "Threshold decision Unknown -> On");
    TEST_ASSERT_NOT_NULL(on);
    TEST_ASSERT_NOT_NULL(strstr(on->message.c_str(), "Controller 3 \"Heater Control\""));
    TEST_ASSERT_NOT_NULL(strstr(on->message.c_str(), "Sensor 1 relative_humidity=72 %"));
    TEST_ASSERT_NOT_NULL(strstr(on->message.c_str(), "on threshold=70 %"));

    clock.now = 250;
    setSnapshot(measurements, 64.0F, 200, 2);
    controller.service();
    const TestLogger::Entry* off = logger.find(LogLevel::Info, "Threshold decision On -> Off");
    TEST_ASSERT_NOT_NULL(off);
    TEST_ASSERT_NOT_NULL(strstr(off->message.c_str(), "Sensor 1 relative_humidity=64 %"));
    TEST_ASSERT_NOT_NULL(strstr(off->message.c_str(), "off threshold=65 %"));
}

void test_unchanged_decision_is_debug_only_and_debug_filtering_is_authoritative() {
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    TestMeasurementResolver measurements;
    setSnapshot(measurements, 68.0F, 0, 1);

    TestLogger debugLogger(LogLevel::Debug);
    ThresholdController debugController(
        thresholdConfiguration(), measurements, actuators, clock, debugLogger,
        2, "Humidity Hold");
    debugController.begin();
    TEST_ASSERT_EQUAL_UINT32(1, debugLogger.count(LogLevel::Debug));
    TEST_ASSERT_EQUAL_UINT32(0, debugLogger.count(LogLevel::Info));

    TestLogger infoLogger(LogLevel::Info);
    ThresholdController infoController(
        thresholdConfiguration(), measurements, actuators, clock, infoLogger,
        2, "Humidity Hold");
    infoController.begin();
    TEST_ASSERT_EQUAL_UINT32(0, infoLogger.count(LogLevel::Debug));
    TEST_ASSERT_EQUAL_UINT32(0, infoLogger.count(LogLevel::Info));
}

void test_source_loss_stale_and_recovery_log_once_per_transition() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    TestMeasurementResolver measurements;
    setSnapshot(measurements, 68.0F, 0, 1);
    ThresholdController controller(
        thresholdConfiguration(), measurements, actuators, clock, logger,
        4, "Ventilation");
    controller.begin();

    measurements.available = false;
    controller.service();
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(1, logger.count(LogLevel::Warn));
    TEST_ASSERT_NOT_NULL(logger.find(LogLevel::Warn,
        "Controller 4 \"Ventilation\": Measurement source Sensor 1 relative_humidity unavailable"));

    measurements.available = true;
    controller.service();
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(1, logger.count(LogLevel::Info));

    clock.now = 1001;
    controller.service();
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(2, logger.count(LogLevel::Warn));
    const TestLogger::Entry* stale = logger.find(LogLevel::Warn, "stale, age=1001 ms, maximum=1000 ms");
    TEST_ASSERT_NOT_NULL(stale);

    setSnapshot(measurements, 68.0F, 1001, 2);
    controller.service();
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(2, logger.count(LogLevel::Info));
}

void test_target_loss_and_retryable_operation_failure_are_suppressed_and_recover() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    TestMeasurementResolver measurements;
    setSnapshot(measurements, 72.0F, 0, 1);
    ThresholdController controller(
        thresholdConfiguration(), measurements, actuators, clock, logger,
        5, "Pump");
    controller.begin();
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(1, logger.count(LogLevel::Warn));
    TEST_ASSERT_NOT_NULL(logger.find(LogLevel::Warn, "target Actuator 1 unavailable"));

    actuators.targets[0] = &actuator;
    actuator.operationResult = ActuatorOperationResult::NotInitialized;
    controller.service();
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(2, logger.count(LogLevel::Warn));
    TEST_ASSERT_NOT_NULL(logger.find(LogLevel::Warn, "could not apply On"));

    actuator.operationResult = ActuatorOperationResult::Completed;
    controller.service();
    controller.service();
    TEST_ASSERT_EQUAL_UINT32(3, logger.count(LogLevel::Info));
    TEST_ASSERT_NOT_NULL(logger.find(LogLevel::Info, "applied On after retry"));
}

void test_blink_phase_cycles_do_not_create_info_log_spam() {
    TestLogger logger;
    TestClock clock;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    BlinkControllerConfiguration configuration;
    configuration.targetActuatorId = 1;
    configuration.onDurationMs = 10;
    configuration.offDurationMs = 10;
    BlinkController controller(configuration, actuators, clock, logger, 6, "Blinker");
    controller.begin();
    for (uint32_t now = 10; now <= 100; now += 10) {
        clock.now = now;
        controller.service();
    }
    TEST_ASSERT_EQUAL_UINT32(0, logger.entries.size());
}

void test_reason_tracks_thresholds_hold_missing_invalid_stale_and_stop() {
    TestLogger logger;
    TestClock clock;
    TestMeasurementResolver measurements;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    ThresholdController controller(thresholdConfiguration(), measurements, actuators, clock, logger);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::NotStarted), static_cast<int>(controller.reason()));
    controller.begin();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::NoMeasurement), static_cast<int>(controller.reason()));
    setSnapshot(measurements, 67, 0, 1);
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::AwaitingThreshold), static_cast<int>(controller.reason()));
    setSnapshot(measurements, 70, 0, 2);
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::OnThreshold), static_cast<int>(controller.reason()));
    TEST_ASSERT_EQUAL_UINT32(1, actuator.setCount);
    setSnapshot(measurements, 67, 0, 3);
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::HysteresisHold), static_cast<int>(controller.reason()));
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::HysteresisHold), static_cast<int>(controller.reason()));
    TEST_ASSERT_EQUAL_UINT32(1, actuator.setCount);
    clock.now = 1001;
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::StaleMeasurement), static_cast<int>(controller.reason()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDecision::On), static_cast<int>(controller.decision()));
    measurements.current = snapshot(0, 1001, 4, false);
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::InvalidMeasurement), static_cast<int>(controller.reason()));
    setSnapshot(measurements, 65, 1001, 5);
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::OffThreshold), static_cast<int>(controller.reason()));
    TEST_ASSERT_EQUAL_UINT32(2, actuator.setCount);
    controller.stop();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::Stopped), static_cast<int>(controller.reason()));
    controller.begin();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::OffThreshold), static_cast<int>(controller.reason()));
}

void test_reason_inverse_direction_and_invalid_configuration() {
    TestLogger logger;
    TestClock clock;
    TestMeasurementResolver measurements;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    auto config = thresholdConfiguration();
    config.direction = ThresholdDirection::OnBelow;
    config.onThreshold = 65;
    config.offThreshold = 70;
    ThresholdController controller(config, measurements, actuators, clock, logger);
    setSnapshot(measurements, 65, 0, 1);
    controller.begin();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::OnThreshold), static_cast<int>(controller.reason()));
    setSnapshot(measurements, 70, 0, 2);
    controller.service();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::OffThreshold), static_cast<int>(controller.reason()));
    config.maxMeasurementAgeMs = 0;
    ThresholdController invalid(config, measurements, actuators, clock, logger);
    invalid.begin();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdReason::InvalidConfiguration), static_cast<int>(invalid.reason()));
}

class TestReasonProvider : public IThresholdReasonProvider {
public:
    ThresholdReason current = ThresholdReason::NotStarted;
    ThresholdReason reason() const override { return current; }
};

void test_enum_property_metadata_codes_and_type_safety() {
    TestReasonProvider provider;
    ControllerPropertyReader reader(3, provider);
    const PropertyReference reference(PropertyComponentKind::Controller, 3, "reason");
    PropertyDescription description;
    TEST_ASSERT_TRUE(reader.describe(reference, description));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyValueKind::Enumeration), static_cast<int>(description.valueKind));
    TEST_ASSERT_EQUAL_UINT32(10, description.enumOptionCount);
    const char* codes[] = {"not_started", "stopped", "invalid_configuration", "no_measurement",
        "invalid_measurement", "stale_measurement", "on_threshold", "off_threshold", "hysteresis_hold", "awaiting_threshold"};
    for (size_t index = 0; index < description.enumOptionCount; ++index) {
        provider.current = static_cast<ThresholdReason>(index);
        PropertySnapshot value;
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::Available), static_cast<int>(reader.read(reference, value)));
        const PropertyEnumOption* option = nullptr;
        TEST_ASSERT_TRUE(value.value.tryGetEnumeration(option));
        TEST_ASSERT_EQUAL_STRING(codes[index], option->stableCode);
        TEST_ASSERT_EQUAL_STRING(description.enumOptions[index].displayText, option->displayText);
        float number = 0;
        bool flag = false;
        uint32_t integer = 0;
        TEST_ASSERT_FALSE(value.value.tryGetFloatingPoint(number));
        TEST_ASSERT_FALSE(value.value.tryGetBoolean(flag));
        TEST_ASSERT_FALSE(value.value.tryGetUnsignedInteger(integer));
        TEST_ASSERT_TRUE(value.valid);
        TEST_ASSERT_FALSE(value.hasQuality);
        TEST_ASSERT_FALSE(value.hasAcceptedMonotonicMs);
        TEST_ASSERT_FALSE(value.hasTimestamp);
        TEST_ASSERT_FALSE(value.hasRevision);
    }
    PropertyValue scalar(MeasurementValue::boolean(true));
    const PropertyEnumOption* option = nullptr;
    TEST_ASSERT_FALSE(scalar.tryGetEnumeration(option));
    provider.current = static_cast<ThresholdReason>(255);
    PropertySnapshot value;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::NoValue), static_cast<int>(reader.read(reference, value)));
    TEST_ASSERT_FALSE(value.valid);
    const PropertyReference wrong[] = {{PropertyComponentKind::Sensor, 3, "reason"},
        {PropertyComponentKind::Controller, 2, "reason"}, {PropertyComponentKind::Controller, 3, nullptr},
        {PropertyComponentKind::Controller, 3, "Reason"}};
    for (const auto& ref : wrong) {
        TEST_ASSERT_FALSE(reader.describe(ref, description));
        TEST_ASSERT_EQUAL_UINT32(0, description.enumOptionCount);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::UnknownReference), static_cast<int>(reader.read(ref, value)));
    }
}

void test_controller_property_web_reads_real_evaluation_without_actuation() {
    TestLogger logger;
    TestClock clock;
    TestMeasurementResolver measurements;
    TestActuator actuator;
    TestActuatorResolver actuators;
    actuators.targets[0] = &actuator;
    ThresholdController controller(thresholdConfiguration(), measurements, actuators, clock, logger);
    setSnapshot(measurements, 70, 0, 1);
    controller.begin();
    ControllerPropertyReader reader(3, controller);
    const PropertyReference reference(PropertyComponentKind::Controller, 3, "reason");
    String html = buildPropertyDiagnosticHtml(reader, reference, 100);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "controller / 3 / reason"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "On threshold reached (on_threshold)"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "<dt>Age</dt><dd>Not available"));
    TEST_ASSERT_EQUAL_UINT32(1, actuator.setCount);
    setSnapshot(measurements, 67, 0, 2);
    controller.service();
    html = buildPropertyDiagnosticHtml(reader, reference, 100);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "hysteresis_hold"));
    TEST_ASSERT_EQUAL_UINT32(1, actuator.setCount);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_begin_handles_missing_stale_invalid_and_initial_in_band_input);
    RUN_TEST(test_begin_commands_on_or_off_at_thresholds_and_accepts_quality_labels);
    RUN_TEST(test_hysteresis_processes_revisions_without_repeating_commands);
    RUN_TEST(test_inverse_hysteresis_switches_on_below_and_off_above);
    RUN_TEST(test_freshness_boundary_wraparound_and_stale_input_do_not_force_off);
    RUN_TEST(test_incompatible_and_invalid_snapshots_are_processed_without_decision);
    RUN_TEST(test_pending_output_retries_without_measurement_and_resolves_replacement);
    RUN_TEST(test_manual_contention_is_not_reconciled_until_decision_changes);
    RUN_TEST(test_cache_clear_removes_source_without_erasing_decision);
    RUN_TEST(test_stop_and_start_reset_decision_and_evaluate_current_snapshot);
    RUN_TEST(test_factory_and_runtime_support_mixed_composition_and_live_rebuild);
    RUN_TEST(test_actuator_runtime_replacement_is_resolved_without_reasserting_output);
    RUN_TEST(test_threshold_debug_evaluation_and_info_transitions_are_self_contained);
    RUN_TEST(test_unchanged_decision_is_debug_only_and_debug_filtering_is_authoritative);
    RUN_TEST(test_source_loss_stale_and_recovery_log_once_per_transition);
    RUN_TEST(test_target_loss_and_retryable_operation_failure_are_suppressed_and_recover);
    RUN_TEST(test_blink_phase_cycles_do_not_create_info_log_spam);
    RUN_TEST(test_reason_tracks_thresholds_hold_missing_invalid_stale_and_stop);
    RUN_TEST(test_reason_inverse_direction_and_invalid_configuration);
    RUN_TEST(test_enum_property_metadata_codes_and_type_safety);
    RUN_TEST(test_controller_property_web_reads_real_evaluation_without_actuation);
    return UNITY_END();
}
