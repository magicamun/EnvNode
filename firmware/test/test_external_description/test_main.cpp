#include <Arduino.h>
#include <unity.h>

#include <climits>
#include <cstring>

#include "ControllerParameterMetadata.h"
#include "ExternalDescriptionBuilder.h"
#include "ExternalMetadata.h"
#include "JsonWriter.h"
#include "MqttTopic.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

void assertContains(const String& value, const char* expected) {
    TEST_ASSERT_NOT_NULL(strstr(value.c_str(), expected));
}

void assertNotContains(const String& value, const char* unexpected) {
    TEST_ASSERT_NULL(strstr(value.c_str(), unexpected));
}

void test_parameter_metadata_is_neutral_stable_and_typed() {
    size_t count = 0;
    const ControllerParameterDescriptor* blink =
        controllerParameterDescriptors(ControllerImplementation::Blink, count);
    TEST_ASSERT_NOT_NULL(blink);
    TEST_ASSERT_EQUAL_UINT32(2, count);
    TEST_ASSERT_EQUAL_STRING("on_duration_ms", blink[0].stableName);
    TEST_ASSERT_EQUAL_STRING("off_duration_ms", blink[1].stableName);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ControllerParameterValueType::UnsignedInteger),
        static_cast<int>(blink[0].valueType));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerParameterUnit::Milliseconds),
        static_cast<int>(blink[0].unit));
    TEST_ASSERT_TRUE(blink[0].writable);
    TEST_ASSERT_TRUE(blink[0].hasMinimum);
    TEST_ASSERT_EQUAL_UINT32(1, blink[0].minimum);
    TEST_ASSERT_TRUE(blink[0].hasMaximum);
    TEST_ASSERT_EQUAL_UINT32(INT32_MAX, blink[0].maximum);

    const ControllerParameterDescriptor* threshold =
        controllerParameterDescriptors(ControllerImplementation::Threshold, count);
    TEST_ASSERT_NOT_NULL(threshold);
    TEST_ASSERT_EQUAL_UINT32(4, count);
    TEST_ASSERT_EQUAL_STRING("on_threshold", threshold[0].stableName);
    TEST_ASSERT_EQUAL_STRING("off_threshold", threshold[1].stableName);
    TEST_ASSERT_EQUAL_STRING("threshold_direction", threshold[2].stableName);
    TEST_ASSERT_EQUAL_STRING("max_measurement_age_ms", threshold[3].stableName);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ControllerParameterValueType::FloatingPoint),
        static_cast<int>(threshold[0].valueType));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ControllerParameterUnit::SourceMeasurementCanonical),
        static_cast<int>(threshold[0].unit));
    TEST_ASSERT_FALSE(threshold[0].hasMinimum);
    TEST_ASSERT_FALSE(threshold[0].hasMaximum);
    TEST_ASSERT_TRUE(threshold[3].hasMinimum);
    TEST_ASSERT_TRUE(threshold[3].hasMaximum);
    TEST_ASSERT_EQUAL_STRING("uint32",
        controllerParameterValueTypeStableName(threshold[3].valueType));
    TEST_ASSERT_EQUAL_STRING("float",
        controllerParameterValueTypeStableName(threshold[0].valueType));
}

void test_external_stable_names_and_mqtt_parameter_names_match() {
    TEST_ASSERT_EQUAL_STRING("on_off",
        actuatorCapabilityStableName(ActuatorCapability::OnOff));
    TEST_ASSERT_EQUAL_STRING("level",
        actuatorCapabilityStableName(ActuatorCapability::Level));
    TEST_ASSERT_EQUAL_STRING("START", controllerRuntimeCommandStableName(
        ControllerRuntimeCommand::Start));
    TEST_ASSERT_EQUAL_STRING("STOP", controllerRuntimeCommandStableName(
        ControllerRuntimeCommand::Stop));
    TEST_ASSERT_EQUAL_STRING("state",
        measurementSemanticsStableName(MeasurementSemantics::State));
    TEST_ASSERT_EQUAL_STRING("event",
        measurementSemanticsStableName(MeasurementSemantics::Event));
    TEST_ASSERT_EQUAL_STRING("float",
        measurementValueKindStableName(ValueKind::FloatingPoint));

    for (uint8_t value = 0;
         value < static_cast<uint8_t>(ControllerParameter::Count);
         ++value) {
        const ControllerParameter parameter = static_cast<ControllerParameter>(value);
        TEST_ASSERT_EQUAL_STRING(
            controllerParameterDescriptor(parameter)->stableName,
            mqttControllerParameterName(parameter));
    }
}

void test_json_string_escaping_handles_quotes_slashes_and_controls() {
    const char source[] = {'q', '"', '\\', '\b', '\f', '\n', '\r', '\t', 1, 0};
    String output;
    appendJsonString(output, source);
    TEST_ASSERT_EQUAL_STRING("\"q\\\"\\\\\\b\\f\\n\\r\\t\\u0001\"", output.c_str());
}

void test_actuator_description_is_configured_contract_only() {
    ActuatorSlotConfiguration slot;
    slot.slotId = 1;
    slot.enabled = true;
    slot.name = "Heat \"relay\"";
    slot.implementation = ActuatorImplementation::GpioOnOff;
    slot.hardware = HardwareResourceAssignment::gpioResource(GpioResource(16));
    String payload;
    TEST_ASSERT_TRUE(buildActuatorDescription(slot, payload));
    TEST_ASSERT_EQUAL_STRING(
        "{\"schema\":1,\"id\":1,\"name\":\"Heat \\\"relay\\\"\","
        "\"enabled\":true,\"implementation\":\"gpio_on_off\","
        "\"implementation_name\":\"GPIO On/Off\","
        "\"capabilities\":[\"on_off\"]}",
        payload.c_str());
    assertNotContains(payload, "GPIO16");
    assertNotContains(payload, "hardware");
    assertNotContains(payload, "initialized");
    assertNotContains(payload, "status");

    slot.enabled = false;
    TEST_ASSERT_TRUE(buildActuatorDescription(slot, payload));
    assertContains(payload, "\"enabled\":false");

    slot.implementation = ActuatorImplementation::None;
    TEST_ASSERT_FALSE(buildActuatorDescription(slot, payload));
    TEST_ASSERT_EQUAL_STRING("", payload.c_str());
}

void test_level_actuator_description_advertises_effective_capabilities() {
    ActuatorSlotConfiguration slot;
    slot.slotId = 2;
    slot.enabled = true;
    slot.name = "Variable output";
    slot.implementation = ActuatorImplementation::GpioPwm;
    slot.hardware = HardwareResourceAssignment::gpioResource(GpioResource(17));
    String payload;
    TEST_ASSERT_TRUE(buildActuatorDescription(slot, payload));
    assertContains(payload, "\"implementation\":\"gpio_pwm\"");
    assertContains(payload, "\"capabilities\":[\"on_off\",\"level\"]");
}

void test_blink_description_contains_target_commands_and_parameters() {
    ControllerSlotConfiguration slot;
    slot.slotId = 1;
    slot.enabled = false;
    slot.name = "Ventilation cycle";
    slot.implementation = ControllerImplementation::Blink;
    slot.implementationConfiguration.blink.targetActuatorId = 3;
    slot.implementationConfiguration.blink.onDurationMs = 1000;
    slot.implementationConfiguration.blink.offDurationMs = 500;
    String payload;
    TEST_ASSERT_TRUE(buildControllerDescription(slot, payload));
    assertContains(payload, "\"schema\":1");
    assertContains(payload, "\"id\":1");
    assertContains(payload, "\"enabled\":false");
    assertContains(payload, "\"implementation\":\"blink\"");
    assertContains(payload, "\"implementation_name\":\"Blink\"");
    assertContains(payload, "\"runtime_commands\":[\"START\",\"STOP\"]");
    assertContains(payload, "\"actuator_id\":3");
    assertContains(payload, "\"required_capabilities\":[\"on_off\"]");
    assertContains(payload, "\"inputs\":[]");
    assertContains(payload, "\"name\":\"on_duration_ms\"");
    assertContains(payload, "\"minimum\":1,\"maximum\":2147483647,\"value\":1000");
    assertContains(payload, "\"name\":\"off_duration_ms\"");
    assertContains(payload, "\"value\":500");
    assertNotContains(payload, "running");
    assertNotContains(payload, "available");
}

void test_threshold_description_uses_measurement_metadata_and_values() {
    ControllerSlotConfiguration slot;
    slot.slotId = 2;
    slot.enabled = true;
    slot.name = "High temperature control";
    slot.implementation = ControllerImplementation::Threshold;
    ThresholdControllerConfiguration& threshold =
        slot.implementationConfiguration.threshold;
    threshold.source = MeasurementSourceReference(1, MeasurementType::Temperature);
    threshold.targetActuatorId = 2;
    threshold.onThreshold = 30.0F;
    threshold.offThreshold = 27.0F;
    threshold.maxMeasurementAgeMs = 15000;
    String payload;
    TEST_ASSERT_TRUE(buildControllerDescription(slot, payload));
    assertContains(payload, "\"implementation\":\"threshold\"");
    assertContains(payload, "\"implementation_name\":\"Threshold / Hysteresis\"");
    assertContains(payload, "\"sensor_id\":1");
    assertContains(payload, "\"measurement\":\"temperature\"");
    assertContains(payload, "\"measurement_name\":\"Temperature\"");
    assertContains(payload, "\"value_type\":\"float\"");
    assertContains(payload, "\"semantics\":\"state\"");
    assertContains(payload, "\"unit\":\"degree_celsius\"");
    assertContains(payload, "\"actuator_id\":2");
    assertContains(payload, "\"name\":\"on_threshold\"");
    assertContains(payload, "\"unit\":\"degree_celsius\",\"writable\":true,\"value\":30");
    assertContains(payload, "\"name\":\"off_threshold\"");
    assertContains(payload, "\"value\":27");
    assertContains(payload, "\"name\":\"max_measurement_age_ms\"");
    assertContains(payload, "\"value\":15000");
    assertNotContains(payload, "\"minimum\":0");

    slot.implementation = ControllerImplementation::None;
    TEST_ASSERT_FALSE(buildControllerDescription(slot, payload));
    TEST_ASSERT_EQUAL_STRING("", payload.c_str());
}

} // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_parameter_metadata_is_neutral_stable_and_typed);
    RUN_TEST(test_external_stable_names_and_mqtt_parameter_names_match);
    RUN_TEST(test_json_string_escaping_handles_quotes_slashes_and_controls);
    RUN_TEST(test_actuator_description_is_configured_contract_only);
    RUN_TEST(test_level_actuator_description_advertises_effective_capabilities);
    RUN_TEST(test_blink_description_contains_target_commands_and_parameters);
    RUN_TEST(test_threshold_description_uses_measurement_metadata_and_values);
    return UNITY_END();
}
