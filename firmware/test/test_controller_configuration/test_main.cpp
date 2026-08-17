#include <unity.h>

#include <climits>
#include <cmath>
#include <limits>

#include "ConfigurationService.h"
#include "ControllerImplementationRegistry.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

struct Fixture {
    ConfigurationService service;

    Fixture() {
        service.loadConfiguration();
        service.resetToDefaults();
    }

    ActuatorSlotConfiguration enabledActuator(ActuatorId id = 1) {
        ActuatorSlotConfiguration slot = service.getConfiguration().actuatorSlots[id - 1];
        slot.enabled = true;
        slot.name = "Target";
        slot.implementation = ActuatorImplementation::GpioOnOff;
        slot.hardware = HardwareResourceAssignment::gpioResource(GpioResource(16));
        return slot;
    }

    ControllerSlotConfiguration blink(ControllerId id = 1, ActuatorId target = 1) {
        ControllerSlotConfiguration slot = service.getConfiguration().controllerSlots[id - 1];
        slot.enabled = true;
        slot.name = "Blink";
        slot.implementation = ControllerImplementation::Blink;
        slot.implementationConfiguration.blink.targetActuatorId = target;
        slot.implementationConfiguration.blink.onDurationMs = 1500;
        slot.implementationConfiguration.blink.offDurationMs = 2500;
        return slot;
    }

    ControllerSlotConfiguration threshold(
        ControllerId id = 1,
        SensorId source = 1,
        MeasurementType type = MeasurementType::Temperature,
        ActuatorId target = 1) {
        ControllerSlotConfiguration slot = service.getConfiguration().controllerSlots[id - 1];
        slot.enabled = true;
        slot.name = "Threshold";
        slot.implementation = ControllerImplementation::Threshold;
        ThresholdControllerConfiguration& configuration =
            slot.implementationConfiguration.threshold;
        configuration.source = MeasurementSourceReference(source, type);
        configuration.targetActuatorId = target;
        configuration.onThreshold = 70.0F;
        configuration.offThreshold = 65.0F;
        configuration.maxMeasurementAgeMs = 15000;
        return slot;
    }

    SensorSlotConfiguration rainGauge(SensorId id = 5) {
        SensorSlotConfiguration slot = service.getConfiguration().sensorSlots[id - 1];
        slot.enabled = true;
        slot.name = "Rain";
        slot.implementation = SensorImplementation::RainGauge;
        slot.schedule = SensorSchedule::eventOnly(true);
        slot.hardware = HardwareResourceAssignment::gpioResource(GpioResource(17));
        slot.implementationConfiguration.rainGauge = RainGaugeConfiguration(
            GpioResource(17), 0.2794F, 50);
        return slot;
    }
};

void test_controller_registry_uses_stable_ids_and_on_off_requirement() {
    const ControllerImplementationMetadata* blink =
        ControllerImplementationRegistry::findByStableId("blink");
    TEST_ASSERT_NOT_NULL(blink);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerImplementation::Blink),
        static_cast<int>(blink->implementation));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ActuatorCapability::OnOff),
        static_cast<int>(blink->requiredActuatorCapabilities));
    TEST_ASSERT_EQUAL_STRING("blink",
        ControllerImplementationRegistry::find(ControllerImplementation::Blink)->stableId);
}

void test_threshold_registry_uses_stable_id_and_on_off_requirement() {
    const ControllerImplementationMetadata* threshold =
        ControllerImplementationRegistry::findByStableId("threshold");
    TEST_ASSERT_NOT_NULL(threshold);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerImplementation::Threshold),
        static_cast<int>(threshold->implementation));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ActuatorCapability::OnOff),
        static_cast<int>(threshold->requiredActuatorCapabilities));
    TEST_ASSERT_EQUAL_STRING("blink",
        ControllerImplementationRegistry::find(ControllerImplementation::Blink)->stableId);
}

void test_default_controller_slots_are_disabled_and_stably_identified() {
    Fixture fixture;
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        const ControllerSlotConfiguration& slot =
            fixture.service.getConfiguration().controllerSlots[index];
        TEST_ASSERT_EQUAL_UINT16(index + 1, slot.slotId);
        TEST_ASSERT_FALSE(slot.enabled);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerImplementation::None),
            static_cast<int>(slot.implementation));
        TEST_ASSERT_EQUAL_UINT32(1000, slot.implementationConfiguration.blink.onDurationMs);
        TEST_ASSERT_EQUAL_UINT32(1000, slot.implementationConfiguration.blink.offDurationMs);
    }
}

void test_valid_blink_targets_enabled_on_off_actuator() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(fixture.blink()));
}

void test_invalid_controller_slot_identity_is_rejected() {
    Fixture fixture;
    ControllerSlotConfiguration slot = fixture.blink();
    slot.slotId = InvalidControllerId;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
}

void test_invalid_or_disabled_or_none_target_is_rejected() {
    Fixture fixture;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(fixture.blink(1, 99)));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(fixture.blink()));

    ActuatorSlotConfiguration none = fixture.service.getConfiguration().actuatorSlots[0];
    none.enabled = true;
    none.implementation = ActuatorImplementation::None;
    none.hardware = HardwareResourceAssignment::none();
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(none));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(fixture.blink()));
}

void test_incompatible_capability_metadata_is_detected() {
    const ControllerImplementationMetadata* blink =
        ControllerImplementationRegistry::find(ControllerImplementation::Blink);
    const ActuatorImplementationMetadata* incompatible =
        ActuatorImplementationRegistry::find(ActuatorImplementation::None);
    TEST_ASSERT_NOT_NULL(blink);
    TEST_ASSERT_NOT_NULL(incompatible);
    TEST_ASSERT_FALSE(hasActuatorCapability(
        incompatible->capabilities, blink->requiredActuatorCapabilities));
}

void test_invalid_blink_durations_are_rejected() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    ControllerSlotConfiguration slot = fixture.blink();
    slot.implementationConfiguration.blink.onDurationMs = 0;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
    slot = fixture.blink();
    slot.implementationConfiguration.blink.offDurationMs = 0;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
    slot = fixture.blink();
    slot.implementationConfiguration.blink.onDurationMs =
        static_cast<uint32_t>(INT32_MAX) + 1U;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
    slot = fixture.blink();
    slot.implementationConfiguration.blink.offDurationMs =
        static_cast<uint32_t>(INT32_MAX) + 1U;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
}

void test_multiple_enabled_controllers_may_share_target() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(fixture.blink(1)));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(fixture.blink(2)));
}

void test_referenced_actuator_cannot_be_disabled_or_made_incompatible() {
    Fixture fixture;
    ActuatorSlotConfiguration actuator = fixture.enabledActuator();
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(actuator));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(fixture.blink()));

    actuator.enabled = false;
    TEST_ASSERT_FALSE(fixture.service.setActuatorSlotConfiguration(actuator));
    actuator.enabled = true;
    actuator.implementation = ActuatorImplementation::None;
    actuator.hardware = HardwareResourceAssignment::none();
    TEST_ASSERT_FALSE(fixture.service.setActuatorSlotConfiguration(actuator));
}

void test_disabled_controller_does_not_constrain_actuator_configuration() {
    Fixture fixture;
    ActuatorSlotConfiguration actuator = fixture.enabledActuator();
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(actuator));
    ControllerSlotConfiguration controller = fixture.blink();
    controller.enabled = false;
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(controller));
    actuator.enabled = false;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(actuator));
}

void test_persistence_uses_stable_id_and_round_trips_blink_parameters() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    ControllerSlotConfiguration saved = fixture.blink();
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(saved));
    TEST_ASSERT_EQUAL_STRING("blink", Preferences::storedString("c1_impl").c_str());

    ConfigurationService loadedService;
    loadedService.loadConfiguration();
    const ControllerSlotConfiguration& loaded =
        loadedService.getConfiguration().controllerSlots[0];
    TEST_ASSERT_TRUE(loaded.enabled);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerImplementation::Blink),
        static_cast<int>(loaded.implementation));
    TEST_ASSERT_EQUAL_UINT16(1, loaded.implementationConfiguration.blink.targetActuatorId);
    TEST_ASSERT_EQUAL_UINT32(1500, loaded.implementationConfiguration.blink.onDurationMs);
    TEST_ASSERT_EQUAL_UINT32(2500, loaded.implementationConfiguration.blink.offDurationMs);
}

void test_valid_threshold_accepts_numeric_state_source_and_on_off_target() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(
        fixture.threshold()));
}

void test_threshold_rejects_invalid_disabled_none_and_unsupported_sources() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(
        fixture.threshold(1, InvalidSensorId)));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(
        fixture.threshold(1, 2, MeasurementType::Temperature)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ValueKind::Boolean),
        static_cast<int>(measurementTypeMetadata(
            MeasurementType::RainDetectorWet).expectedValueKind));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(
        fixture.threshold(1, 1, MeasurementType::RainDetectorWet)));

    SensorSlotConfiguration source = fixture.service.getConfiguration().sensorSlots[0];
    source.enabled = false;
    source.schedule.enabled = false;
    TEST_ASSERT_TRUE(fixture.service.setSensorSlotConfiguration(source));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(fixture.threshold()));

    source.implementation = SensorImplementation::None;
    source.hardware = HardwareResourceAssignment::none();
    source.schedule = SensorSchedule::eventOnly(false);
    TEST_ASSERT_TRUE(fixture.service.setSensorSlotConfiguration(source));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(fixture.threshold()));
}

void test_threshold_rejects_event_and_numeric_non_state_measurements() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    TEST_ASSERT_TRUE(fixture.service.setSensorSlotConfiguration(fixture.rainGauge()));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(
        fixture.threshold(1, 5, MeasurementType::RainGaugeTip)));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(
        fixture.threshold(1, 5, MeasurementType::RainfallIncrement)));
}

void test_threshold_values_must_be_finite_and_strictly_ordered() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    ControllerSlotConfiguration slot = fixture.threshold();
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(slot));

    slot.implementationConfiguration.threshold.onThreshold =
        std::numeric_limits<float>::quiet_NaN();
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
    slot = fixture.threshold();
    slot.implementationConfiguration.threshold.onThreshold =
        std::numeric_limits<float>::infinity();
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
    slot = fixture.threshold();
    slot.implementationConfiguration.threshold.offThreshold =
        -std::numeric_limits<float>::infinity();
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
    slot = fixture.threshold();
    slot.implementationConfiguration.threshold.offThreshold = 70.0F;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
    slot.implementationConfiguration.threshold.offThreshold = 71.0F;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
}

void test_threshold_maximum_measurement_age_is_wrap_safe() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    ControllerSlotConfiguration slot = fixture.threshold();
    slot.implementationConfiguration.threshold.maxMeasurementAgeMs = 0;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
    slot.implementationConfiguration.threshold.maxMeasurementAgeMs = INT32_MAX;
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(slot));
    slot.implementationConfiguration.threshold.maxMeasurementAgeMs =
        static_cast<uint32_t>(INT32_MAX) + 1U;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
}

void test_threshold_rejects_invalid_disabled_and_none_targets() {
    Fixture fixture;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(
        fixture.threshold(1, 1, MeasurementType::Temperature, 99)));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(fixture.threshold()));
    ActuatorSlotConfiguration none = fixture.service.getConfiguration().actuatorSlots[0];
    none.enabled = true;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(none));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(fixture.threshold()));
}

void test_threshold_reverse_sensor_integrity_protects_only_referenced_source() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(fixture.threshold()));

    SensorSlotConfiguration source = fixture.service.getConfiguration().sensorSlots[0];
    source.enabled = false;
    source.schedule.enabled = false;
    TEST_ASSERT_FALSE(fixture.service.setSensorSlotConfiguration(source));
    source = fixture.service.getConfiguration().sensorSlots[0];
    source.implementation = SensorImplementation::SimulatedHumidity;
    TEST_ASSERT_FALSE(fixture.service.setSensorSlotConfiguration(source));

    SensorSlotConfiguration unrelated = fixture.service.getConfiguration().sensorSlots[1];
    unrelated.schedule.sampleIntervalMs = 6000;
    TEST_ASSERT_TRUE(fixture.service.setSensorSlotConfiguration(unrelated));
}

void test_disabled_threshold_does_not_constrain_sensor_or_actuator_changes() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    ControllerSlotConfiguration controller = fixture.threshold();
    controller.enabled = false;
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(controller));

    SensorSlotConfiguration source = fixture.service.getConfiguration().sensorSlots[0];
    source.enabled = false;
    source.schedule.enabled = false;
    TEST_ASSERT_TRUE(fixture.service.setSensorSlotConfiguration(source));
    ActuatorSlotConfiguration target = fixture.service.getConfiguration().actuatorSlots[0];
    target.enabled = false;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(target));
}

void test_threshold_reverse_actuator_integrity_protects_only_referenced_target() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    ActuatorSlotConfiguration unrelated = fixture.enabledActuator(2);
    unrelated.hardware = HardwareResourceAssignment::gpioResource(GpioResource(17));
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(unrelated));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(fixture.threshold()));

    ActuatorSlotConfiguration target = fixture.service.getConfiguration().actuatorSlots[0];
    target.enabled = false;
    TEST_ASSERT_FALSE(fixture.service.setActuatorSlotConfiguration(target));
    target.enabled = true;
    target.implementation = ActuatorImplementation::None;
    target.hardware = HardwareResourceAssignment::none();
    TEST_ASSERT_FALSE(fixture.service.setActuatorSlotConfiguration(target));

    unrelated.enabled = false;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(unrelated));
}

void test_threshold_persistence_round_trips_stable_source_and_parameters() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    ControllerSlotConfiguration saved = fixture.threshold();
    saved.implementationConfiguration.threshold.onThreshold = 23.5F;
    saved.implementationConfiguration.threshold.offThreshold = 21.25F;
    saved.implementationConfiguration.threshold.maxMeasurementAgeMs = 30000;
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(saved));
    TEST_ASSERT_EQUAL_STRING("threshold", Preferences::storedString("c1_impl").c_str());
    TEST_ASSERT_EQUAL_STRING("temperature", Preferences::storedString("c1_type").c_str());

    ConfigurationService loadedService;
    loadedService.loadConfiguration();
    const ControllerSlotConfiguration& loaded =
        loadedService.getConfiguration().controllerSlots[0];
    const ThresholdControllerConfiguration& threshold =
        loaded.implementationConfiguration.threshold;
    TEST_ASSERT_TRUE(loaded.enabled);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerImplementation::Threshold),
        static_cast<int>(loaded.implementation));
    TEST_ASSERT_EQUAL_UINT16(1, threshold.source.sensorId);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::Temperature),
        static_cast<int>(threshold.source.measurementType));
    TEST_ASSERT_EQUAL_UINT16(1, threshold.targetActuatorId);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 23.5F, threshold.onThreshold);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 21.25F, threshold.offThreshold);
    TEST_ASSERT_EQUAL_UINT32(30000, threshold.maxMeasurementAgeMs);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_controller_registry_uses_stable_ids_and_on_off_requirement);
    RUN_TEST(test_threshold_registry_uses_stable_id_and_on_off_requirement);
    RUN_TEST(test_default_controller_slots_are_disabled_and_stably_identified);
    RUN_TEST(test_valid_blink_targets_enabled_on_off_actuator);
    RUN_TEST(test_invalid_controller_slot_identity_is_rejected);
    RUN_TEST(test_invalid_or_disabled_or_none_target_is_rejected);
    RUN_TEST(test_incompatible_capability_metadata_is_detected);
    RUN_TEST(test_invalid_blink_durations_are_rejected);
    RUN_TEST(test_multiple_enabled_controllers_may_share_target);
    RUN_TEST(test_referenced_actuator_cannot_be_disabled_or_made_incompatible);
    RUN_TEST(test_disabled_controller_does_not_constrain_actuator_configuration);
    RUN_TEST(test_persistence_uses_stable_id_and_round_trips_blink_parameters);
    RUN_TEST(test_valid_threshold_accepts_numeric_state_source_and_on_off_target);
    RUN_TEST(test_threshold_rejects_invalid_disabled_none_and_unsupported_sources);
    RUN_TEST(test_threshold_rejects_event_and_numeric_non_state_measurements);
    RUN_TEST(test_threshold_values_must_be_finite_and_strictly_ordered);
    RUN_TEST(test_threshold_maximum_measurement_age_is_wrap_safe);
    RUN_TEST(test_threshold_rejects_invalid_disabled_and_none_targets);
    RUN_TEST(test_threshold_reverse_sensor_integrity_protects_only_referenced_source);
    RUN_TEST(test_disabled_threshold_does_not_constrain_sensor_or_actuator_changes);
    RUN_TEST(test_threshold_reverse_actuator_integrity_protects_only_referenced_target);
    RUN_TEST(test_threshold_persistence_round_trips_stable_source_and_parameters);
    return UNITY_END();
}
