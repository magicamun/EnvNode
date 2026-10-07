#include <Arduino.h>
#include <unity.h>
#include <cstring>
#include "ActuatorPropertyReader.h"
#include "PropertyWebView.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

class TestActuator : public IOnOffActuator {
public:
    ActuatorOperationResult begin() override { ++writes; return ActuatorOperationResult::Completed; }
    ActuatorOperationResult shutdown() override { ++writes; return ActuatorOperationResult::Completed; }
    ActuatorOperationResult setState(OnOffState) override { ++writes; return ActuatorOperationResult::Completed; }
    OnOffState state() const override { ++reads; return output; }
    bool initialized() const override { return ready; }
    bool ready = true;
    OnOffState output = OnOffState::Off;
    unsigned writes = 0;
    mutable unsigned reads = 0;
};

const PropertyReference State(PropertyComponentKind::Actuator, 2, "state");

void test_reads_live_boolean_state_without_writing_or_inventing_metadata() {
    TestActuator actuator;
    ActuatorPropertyReader adapter(2, actuator);
    const IPropertyReader& reader = adapter;
    PropertyDescription description;
    TEST_ASSERT_TRUE(reader.describe(State, description));
    TEST_ASSERT_EQUAL_STRING("state", description.stableKey);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyValueKind::Boolean), static_cast<int>(description.valueKind));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PresentationUnit::None), static_cast<int>(description.canonicalUnit));
    PropertySnapshot result;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::Available), static_cast<int>(reader.read(State, result)));
    bool value = true;
    TEST_ASSERT_TRUE(result.value.tryGetBoolean(value));
    TEST_ASSERT_FALSE(value);
    actuator.output = OnOffState::On;
    reader.read(State, result);
    TEST_ASSERT_TRUE(result.value.tryGetBoolean(value));
    TEST_ASSERT_TRUE(value);
    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_FALSE(result.hasTimestamp);
    TEST_ASSERT_FALSE(result.hasAcceptedMonotonicMs);
    TEST_ASSERT_FALSE(result.hasRevision);
    TEST_ASSERT_FALSE(result.hasQuality);
    TEST_ASSERT_EQUAL_UINT32(0, actuator.writes);
}

void test_uninitialized_output_has_no_value_and_clears_old_state() {
    TestActuator actuator;
    actuator.output = OnOffState::On;
    ActuatorPropertyReader reader(2, actuator);
    PropertySnapshot result;
    reader.read(State, result);
    actuator.ready = false;
    PropertyDescription description;
    TEST_ASSERT_TRUE(reader.describe(State, description));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::NoValue), static_cast<int>(reader.read(State, result)));
    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyValueKind::None), static_cast<int>(result.value.kind()));
    TEST_ASSERT_EQUAL_UINT32(1, actuator.reads);
    TEST_ASSERT_EQUAL_UINT32(0, actuator.writes);
}

void test_unknown_references_are_rejected_without_reading_output() {
    TestActuator actuator;
    ActuatorPropertyReader reader(2, actuator);
    const PropertyReference references[] = {
        {PropertyComponentKind::Sensor, 2, "state"},
        {PropertyComponentKind::Actuator, 1, "state"},
        {PropertyComponentKind::Actuator, 0, "state"},
        {PropertyComponentKind::Actuator, 2, nullptr},
        {PropertyComponentKind::Actuator, 2, "State"},
        {PropertyComponentKind::Actuator, 2, ""},
    };
    for (const PropertyReference& reference : references) {
        PropertyDescription description;
        reader.describe(State, description);
        TEST_ASSERT_FALSE(reader.describe(reference, description));
        TEST_ASSERT_EQUAL_STRING("", description.stableKey);
        PropertySnapshot result;
        TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::UnknownReference), static_cast<int>(reader.read(reference, result)));
        TEST_ASSERT_FALSE(result.valid);
    }
    ActuatorPropertyReader invalid(0, actuator);
    PropertySnapshot result;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::UnknownReference),
        static_cast<int>(invalid.read({PropertyComponentKind::Actuator, 0, "state"}, result)));
    TEST_ASSERT_EQUAL_UINT32(0, actuator.reads);
}

void test_web_displays_on_off_and_marks_age_unavailable() {
    TestActuator actuator;
    ActuatorPropertyReader reader(2, actuator);
    String html = buildPropertyDiagnosticHtml(reader, State, 123456);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "actuator / 2 / state"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "<dd>Off</dd>"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "<dt>Age</dt><dd>Not available"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "<dt>Quality</dt><dd>Not available"));
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "no mechanical feedback"));
    actuator.output = OnOffState::On;
    html = buildPropertyDiagnosticHtml(reader, State, 123456);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "<dd>On</dd>"));
    actuator.ready = false;
    html = buildPropertyDiagnosticHtml(reader, State, 123456);
    TEST_ASSERT_NOT_NULL(strstr(html.c_str(), "State unavailable"));
    TEST_ASSERT_NULL(strstr(html.c_str(), "<dd>On</dd>"));
    TEST_ASSERT_EQUAL_UINT32(0, actuator.writes);
}

class LevelActuator : public ILevelActuator {
public:
    bool ready=true; unsigned writes=0; ActuatorLevel output;
    ActuatorOperationResult begin() override { ++writes; return ActuatorOperationResult::Completed; }
    ActuatorOperationResult shutdown() override { ++writes; return ActuatorOperationResult::Completed; }
    ActuatorOperationResult setState(OnOffState) override { ++writes; return ActuatorOperationResult::Completed; }
    ActuatorOperationResult setLevel(ActuatorLevel) override { ++writes; return ActuatorOperationResult::Completed; }
    OnOffState state() const override { return output.percent() ? OnOffState::On : OnOffState::Off; }
    bool initialized() const override { return ready; }
    ActuatorLevel level() const override { return output; }
};
void test_level_is_read_only_percent_and_requires_capability() {
    LevelActuator actuator; ActuatorPropertyReader reader(2,actuator,&actuator);
    PropertyReference level(PropertyComponentKind::Actuator,2,"level");
    PropertyDescription description; TEST_ASSERT_TRUE(reader.describe(level,description));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PresentationUnit::Percent),static_cast<int>(description.canonicalUnit));
    for (uint8_t expected : {0,37,100}) {
        ActuatorLevel::tryCreate(expected,actuator.output);
        PropertySnapshot snapshot; TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::Available),static_cast<int>(reader.read(level,snapshot)));
        uint32_t value=999; TEST_ASSERT_TRUE(snapshot.value.tryGetUnsignedInteger(value)); TEST_ASSERT_EQUAL_UINT32(expected,value);
    }
    TEST_ASSERT_EQUAL_UINT32(0,actuator.writes);
    actuator.ready=false; PropertySnapshot snapshot;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PropertyReadResult::NoValue),static_cast<int>(reader.read(level,snapshot)));
    TestActuator relay; ActuatorPropertyReader relayReader(2,relay); TEST_ASSERT_FALSE(relayReader.describe(level,description));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_level_is_read_only_percent_and_requires_capability);
    RUN_TEST(test_reads_live_boolean_state_without_writing_or_inventing_metadata);
    RUN_TEST(test_uninitialized_output_has_no_value_and_clears_old_state);
    RUN_TEST(test_unknown_references_are_rejected_without_reading_output);
    RUN_TEST(test_web_displays_on_off_and_marks_age_unavailable);
    return UNITY_END();
}
