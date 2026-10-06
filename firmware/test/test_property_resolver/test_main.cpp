#include <unity.h>
#include <cstring>
#include "PropertyResolver.h"
#include "PropertyWebView.h"
#include "PropertyTextFormatter.h"
#include "SensorManager.h"
#include "MeasurementSnapshotCache.h"
#include "ActuatorRuntime.h"
#include "ControllerRuntime.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
unsigned gpioWrites = 0;
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) { ++gpioWrites; }

class Clock : public IMonotonicClock {
public: uint32_t nowMs() const override { return 0; }
};
class Time : public ITimeService {
public:
    void begin() override {}
    void loop() override {}
    bool synchronized() const override { return true; }
    time_t now() const override { return 1; }
    bool localCivilTime(tm&) const override { return false; }
    String iso8601Utc() const override { return ""; }
    String iso8601Local() const override { return ""; }
    String iso8601Local(time_t) const override { return ""; }
    uint32_t epoch() const override { return 1; }
};
class Sink : public IMeasurementSink {
public: void emit(const Measurement&) override {}
};
class Log : public ILogger {
public:
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
};
class Sensor : public ISensor {
public:
    SensorId id() const override { return 1; }
    const char* type() const override { return "test"; }
    SensorProvenance provenance() const override { return SensorProvenance::Simulated; }
    SensorState state() const override { return SensorState::Ready; }
    bool supports(MeasurementType type) const override { return type == MeasurementType::Temperature; }
    void begin() override {}
    SensorOperationResult service(IMeasurementSink&) override { return SensorOperationResult::NoData; }
    SensorOperationResult sample(IMeasurementSink&) override { return SensorOperationResult::NoData; }
};

struct Fixture {
    Clock clock;
    Time time;
    Log log;
    Sink sink;
    Sensor sensor;
    MeasurementSnapshotCache measurements;
    SensorManager sensors{time, clock, sink, measurements};
    ActuatorFactory actuatorFactory{log};
    ActuatorRuntime actuators{actuatorFactory, log};
    ControllerFactory controllerFactory{measurements, actuators, clock, log};
    ControllerRuntime controllers{controllerFactory, log};
    PropertyResolver resolver{sensors, measurements, actuators, controllers};
    ActuatorSlotConfiguration actuatorSlots[MaxActuatorSlotCount];
    ControllerSlotConfiguration controllerSlots[MaxControllerSlotCount];

    Fixture() {
        sensors.registerSensor(sensor, SensorSchedule::periodic(1000));
        for (size_t i = 0; i < MaxActuatorSlotCount; ++i) {
            actuatorSlots[i].slotId = i + 1;
            actuatorSlots[i].name = "Unused";
        }
        actuatorSlots[0].enabled = true;
        actuatorSlots[0].implementation = ActuatorImplementation::GpioOnOff;
        actuatorSlots[0].hardware = HardwareResourceAssignment::gpioResource(GpioResource(16));
        actuators.initialize(actuatorSlots);
        for (size_t i = 0; i < MaxControllerSlotCount; ++i) {
            controllerSlots[i].slotId = i + 1;
            controllerSlots[i].name = "Unused";
        }
        controllerSlots[0].enabled = true;
        controllerSlots[0].implementation = ControllerImplementation::Threshold;
        auto& config = controllerSlots[0].implementationConfiguration.threshold;
        config.source = MeasurementSourceReference(1, MeasurementType::Temperature);
        config.targetActuatorId = 1;
        config.direction = ThresholdDirection::OnAbove;
        config.onThreshold = 25;
        config.offThreshold = 20;
        config.maxMeasurementAgeMs = 1000;
        controllers.initialize(controllerSlots);
    }
    void sample(float number, bool valid = true) {
        Measurement measurement;
        measurement.source = 1;
        measurement.type = MeasurementType::Temperature;
        measurement.timestamp = 1;
        measurement.valid = valid;
        measurement.value = valid ? MeasurementValue::floatingPoint(number) : MeasurementValue::none();
        measurements.observe(measurement, 0);
    }
};

const PropertyReference Temperature(PropertyComponentKind::Sensor, 1, "temperature");
const PropertyReference Output(PropertyComponentKind::Actuator, 1, "state");
const PropertyReference Reason(PropertyComponentKind::Controller, 1, "reason");

void test_routes_all_categories_with_same_id_and_preserves_types() {
    Fixture f;
    const IPropertyReader& reader = f.resolver;
    PropertySnapshot value;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::NoValue), static_cast<int>(reader.read(Temperature, value)));
    f.sample(26);
    f.controllers.loop();
    const unsigned writes = gpioWrites;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::Available), static_cast<int>(reader.read(Temperature, value)));
    float number = 0;
    TEST_ASSERT_TRUE(value.value.tryGetFloatingPoint(number));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 26, number);
    TEST_ASSERT_TRUE(value.hasTimestamp);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::Available), static_cast<int>(reader.read(Output, value)));
    bool on = false;
    TEST_ASSERT_TRUE(value.value.tryGetBoolean(on));
    TEST_ASSERT_TRUE(on);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::Available), static_cast<int>(reader.read(Reason, value)));
    const PropertyEnumOption* option = nullptr;
    TEST_ASSERT_TRUE(value.value.tryGetEnumeration(option));
    TEST_ASSERT_EQUAL_STRING("on_threshold", option->stableCode);
    const PropertyReference refs[] = {Temperature, Output, Reason};
    for (const auto& ref : refs) {
        PropertyDescription description;
        TEST_ASSERT_TRUE(reader.describe(ref, description));
        TEST_ASSERT_EQUAL_STRING(ref.propertyKey, description.stableKey);
        TEST_ASSERT_NOT_NULL(strstr(buildPropertyDiagnosticHtml(reader, ref, 100).c_str(), ref.propertyKey));
    }
    TEST_ASSERT_EQUAL_STRING("Temperature: 26.0 C", formatPropertyText(reader, Temperature, "Temperature: %.1f C").text);
    TEST_ASSERT_EQUAL_STRING("Valve: On", formatPropertyText(reader, Output, "Valve: %s").text);
    TEST_ASSERT_EQUAL_STRING("Reason: On threshold reached", formatPropertyText(reader, Reason, "Reason: %s").text);
    TEST_ASSERT_EQUAL_UINT32(writes, gpioWrites);
    f.sample(0, false);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::Available), static_cast<int>(reader.read(Temperature, value)));
    TEST_ASSERT_FALSE(value.valid);
}

void test_unknown_reference_resets_previous_output() {
    Fixture f;
    const PropertyReference unknown[] = {
        {PropertyComponentKind::Unknown, 1, "state"},
        {PropertyComponentKind::Sensor, 2, "temperature"},
        {PropertyComponentKind::Actuator, 2, "state"},
        {PropertyComponentKind::Controller, 2, "reason"},
        {PropertyComponentKind::Sensor, 1, nullptr},
        {PropertyComponentKind::Actuator, 1, "reason"},
        {PropertyComponentKind::Controller, 1, "state"},
        {PropertyComponentKind::Sensor, 0, "temperature"},
    };
    for (const auto& ref : unknown) {
        PropertyDescription description;
        f.resolver.describe(Reason, description);
        TEST_ASSERT_FALSE(f.resolver.describe(ref, description));
        TEST_ASSERT_EQUAL_STRING("", description.stableKey);
        TEST_ASSERT_EQUAL_UINT32(0, description.enumOptionCount);
        PropertySnapshot value;
        f.resolver.read(Reason, value);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::UnknownReference), static_cast<int>(f.resolver.read(ref, value)));
        TEST_ASSERT_FALSE(value.valid);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyValueKind::None), static_cast<int>(value.value.kind()));
    }
}

void test_same_resolver_survives_removal_and_replacement() {
    Fixture f;
    PropertySnapshot value;
    f.sample(26);
    f.controllers.loop();
    f.sensors.clear();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::UnknownReference), static_cast<int>(f.resolver.read(Temperature, value)));
    Sensor replacement;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorRegistrationResult::Registered),
        static_cast<int>(f.sensors.registerSensor(replacement, SensorSchedule::periodic(1000))));
    f.sample(19);
    f.resolver.read(Temperature, value);
    float number = 0;
    TEST_ASSERT_TRUE(value.value.tryGetFloatingPoint(number));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 19, number);

    f.actuatorSlots[0].enabled = false;
    TEST_ASSERT_TRUE(f.actuators.rebuild(f.actuatorSlots));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::UnknownReference), static_cast<int>(f.resolver.read(Output, value)));
    f.actuatorSlots[0].enabled = true;
    TEST_ASSERT_TRUE(f.actuators.rebuild(f.actuatorSlots));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::Available), static_cast<int>(f.resolver.read(Output, value)));
    bool on = true;
    TEST_ASSERT_TRUE(value.value.tryGetBoolean(on));
    TEST_ASSERT_FALSE(on);

    f.controllerSlots[0].enabled = false;
    TEST_ASSERT_TRUE(f.controllers.rebuild(f.controllerSlots));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::UnknownReference), static_cast<int>(f.resolver.read(Reason, value)));
    f.controllerSlots[0].enabled = true;
    TEST_ASSERT_TRUE(f.controllers.rebuild(f.controllerSlots));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::Available), static_cast<int>(f.resolver.read(Reason, value)));
    const PropertyEnumOption* option = nullptr;
    TEST_ASSERT_TRUE(value.value.tryGetEnumeration(option));
    TEST_ASSERT_EQUAL_STRING("off_threshold", option->stableCode);
    // A different implementation in the same slot must not keep its old reason binding.
    f.controllerSlots[0].implementation = ControllerImplementation::Blink;
    auto& blink = f.controllerSlots[0].implementationConfiguration.blink;
    blink.targetActuatorId = 1;
    blink.onDurationMs = 100;
    blink.offDurationMs = 100;
    TEST_ASSERT_TRUE(f.controllers.rebuild(f.controllerSlots));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::UnknownReference), static_cast<int>(f.resolver.read(Reason, value)));
    // Drop the borrowed local sensor before its lifetime ends.
    f.sensors.clear();
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_routes_all_categories_with_same_id_and_preserves_types);
    RUN_TEST(test_unknown_reference_resets_previous_output);
    RUN_TEST(test_same_resolver_survives_removal_and_replacement);
    return UNITY_END();
}
