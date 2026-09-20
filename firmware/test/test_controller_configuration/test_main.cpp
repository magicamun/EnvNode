#include <unity.h>

#include <climits>
#include <cmath>
#include <limits>

#include "ConfigurationService.h"
#include "ControllerImplementationRegistry.h"
#include "ControllerWebSupport.h"

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

void test_threshold_direction_exposes_matching_comparison_symbols() {
    TEST_ASSERT_EQUAL_STRING("≥",
        thresholdOnComparisonSymbol(ThresholdDirection::OnAbove));
    TEST_ASSERT_EQUAL_STRING("≤",
        thresholdOffComparisonSymbol(ThresholdDirection::OnAbove));
    TEST_ASSERT_EQUAL_STRING("≤",
        thresholdOnComparisonSymbol(ThresholdDirection::OnBelow));
    TEST_ASSERT_EQUAL_STRING("≥",
        thresholdOffComparisonSymbol(ThresholdDirection::OnBelow));
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

void test_on_off_controller_accepts_level_actuator_and_persistence_round_trips() {
    Fixture fixture;
    ActuatorSlotConfiguration actuator = fixture.enabledActuator();
    actuator.implementation = ActuatorImplementation::GpioPwm;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(actuator));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(fixture.blink()));
    TEST_ASSERT_EQUAL_STRING("gpio_pwm", Preferences::storedString("a1_impl").c_str());

    ConfigurationService loadedService;
    loadedService.loadConfiguration();
    const ActuatorSlotConfiguration& loaded =
        loadedService.getConfiguration().actuatorSlots[0];
    TEST_ASSERT_TRUE(loaded.enabled);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ActuatorImplementation::GpioPwm),
        static_cast<int>(loaded.implementation));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HardwareResourceKind::GPIO),
        static_cast<int>(loaded.hardware.kind));
    TEST_ASSERT_EQUAL_UINT8(16, loaded.hardware.gpio.number);
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

void test_enabled_controllers_must_exclusively_own_their_targets() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(fixture.blink(1)));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(fixture.blink(2)));
    const ControllerSlotConfiguration& rejected =
        fixture.service.getConfiguration().controllerSlots[1];
    TEST_ASSERT_FALSE(rejected.enabled);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerImplementation::None),
        static_cast<int>(rejected.implementation));
}

void test_disabled_controller_does_not_claim_target_and_enable_is_revalidated() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(fixture.blink(1)));
    ControllerSlotConfiguration second = fixture.blink(2);
    second.enabled = false;
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(second));
    second.enabled = true;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(second));

    ControllerSlotConfiguration first = fixture.blink(1);
    first.enabled = false;
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(first));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(second));
}

void test_target_exclusivity_applies_across_controller_implementations() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(fixture.blink(1)));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(
        fixture.threshold(2, 1, MeasurementType::Temperature, 1)));

    ControllerSlotConfiguration switched = fixture.threshold(
        1, 1, MeasurementType::Temperature, 1);
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(switched));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ControllerImplementation::Threshold),
        static_cast<int>(fixture.service.getConfiguration().controllerSlots[0].implementation));
}

void test_enabled_controllers_may_own_different_targets() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator(1)));
    ActuatorSlotConfiguration secondActuator = fixture.enabledActuator(2);
    secondActuator.hardware =
        HardwareResourceAssignment::gpioResource(GpioResource(17));
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(secondActuator));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(fixture.blink(1, 1)));
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(
        fixture.threshold(2, 1, MeasurementType::Temperature, 2)));
}

void test_controller_web_target_filter_excludes_other_claim_but_keeps_own_claim() {
    Fixture fixture;
    ControllerSlotConfiguration slots[2] = {
        fixture.blink(1, 2), fixture.threshold(
            2, 1, MeasurementType::Temperature, 3)};
    TEST_ASSERT_FALSE(isControllerTargetClaimedByOtherEnabledSlot(
        slots, 2, 1, 2));
    TEST_ASSERT_TRUE(isControllerTargetClaimedByOtherEnabledSlot(
        slots, 2, 2, 2));
    slots[0].enabled = false;
    TEST_ASSERT_FALSE(isControllerTargetClaimedByOtherEnabledSlot(
        slots, 2, 2, 2));
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

void test_module_target_persists_without_current_runtime_actuator() {
    Fixture fixture;
    ControllerSlotConfiguration saved = fixture.blink();
    BlinkControllerConfiguration& blink = saved.implementationConfiguration.blink;
    blink.targetActuatorId = InvalidActuatorId;
    uint8_t instanceId[16];
    for (size_t index = 0; index < sizeof(instanceId); ++index) {
        instanceId[index] = static_cast<uint8_t>(0xa0 + index);
    }
    setModuleActuatorInstanceId(saved.moduleTarget, instanceId);
    setModuleActuatorDeviceId(saved.moduleTarget, "relay.2");
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(saved));

    ConfigurationService loadedService;
    loadedService.loadConfiguration();
    const BlinkControllerConfiguration& loaded =
        loadedService.getConfiguration().controllerSlots[0]
            .implementationConfiguration.blink;
    TEST_ASSERT_TRUE(validModuleActuatorReference(
        loadedService.getConfiguration().controllerSlots[0].moduleTarget));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(saved.moduleTarget.deviceIdHash,
        loadedService.getConfiguration().controllerSlots[0].moduleTarget.deviceIdHash, 4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(saved.moduleTarget.moduleInstanceFingerprint,
        loadedService.getConfiguration().controllerSlots[0].moduleTarget.moduleInstanceFingerprint, 12);
}

void test_enabled_controllers_exclusively_claim_same_module_device() {
    Fixture fixture;
    ControllerSlotConfiguration first = fixture.blink(1);
    first.implementationConfiguration.blink.targetActuatorId = InvalidActuatorId;
    memset(first.moduleTarget.moduleInstanceFingerprint, 0x5a,
        sizeof(first.moduleTarget.moduleInstanceFingerprint));
    setModuleActuatorDeviceId(first.moduleTarget, "relay.1");
    TEST_ASSERT_TRUE(fixture.service.setControllerSlotConfiguration(first));

    ControllerSlotConfiguration second = fixture.blink(2);
    second.implementationConfiguration.blink.targetActuatorId = InvalidActuatorId;
    second.moduleTarget = first.moduleTarget;
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(second));
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
    saved.implementationConfiguration.threshold.onThreshold = 21.25F;
    saved.implementationConfiguration.threshold.offThreshold = 23.5F;
    saved.implementationConfiguration.threshold.direction = ThresholdDirection::OnBelow;
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
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 21.25F, threshold.onThreshold);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 23.5F, threshold.offThreshold);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ThresholdDirection::OnBelow),
        static_cast<int>(threshold.direction));
    TEST_ASSERT_EQUAL_UINT32(30000, threshold.maxMeasurementAgeMs);
}

void test_threshold_web_filters_measurements_sensors_and_actuators_by_metadata() {
    TEST_ASSERT_TRUE(isThresholdCompatibleMeasurementType(MeasurementType::Temperature));
    TEST_ASSERT_TRUE(isThresholdCompatibleMeasurementType(
        MeasurementType::RelativeHumidity));
    TEST_ASSERT_FALSE(isThresholdCompatibleMeasurementType(
        MeasurementType::RainDetectorWet));
    TEST_ASSERT_FALSE(isThresholdCompatibleMeasurementType(
        MeasurementType::RainGaugeTip));
    TEST_ASSERT_FALSE(isThresholdCompatibleMeasurementType(
        MeasurementType::RainfallIncrement));

    Fixture fixture;
    const SensorSlotConfiguration& am2302 =
        fixture.service.getConfiguration().sensorSlots[3];
    TEST_ASSERT_TRUE(isEligibleThresholdSensor(am2302));
    TEST_ASSERT_TRUE(sensorSupportsThresholdMeasurement(
        am2302, MeasurementType::Temperature));
    TEST_ASSERT_TRUE(sensorSupportsThresholdMeasurement(
        am2302, MeasurementType::RelativeHumidity));
    TEST_ASSERT_FALSE(sensorSupportsThresholdMeasurement(
        am2302, MeasurementType::RainDetectorWet));
    SensorSlotConfiguration rain = fixture.rainGauge();
    TEST_ASSERT_FALSE(isEligibleThresholdSensor(rain));

    ActuatorSlotConfiguration actuator = fixture.enabledActuator();
    TEST_ASSERT_TRUE(isEligibleThresholdActuator(actuator));
    actuator.enabled = false;
    TEST_ASSERT_FALSE(isEligibleThresholdActuator(actuator));
    actuator.enabled = true;
    actuator.implementation = ActuatorImplementation::None;
    TEST_ASSERT_FALSE(isEligibleThresholdActuator(actuator));
}

void test_threshold_web_measurements_are_scoped_to_selected_sensor_implementation() {
    Fixture fixture;
    SensorSlotConfiguration temperature =
        fixture.service.getConfiguration().sensorSlots[0];
    MeasurementType types[MaxImplementationMeasurementTypeCount];
    size_t count = thresholdMeasurementTypesForSensor(
        temperature, types, MaxImplementationMeasurementTypeCount);
    TEST_ASSERT_EQUAL_UINT32(1, count);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::Temperature),
        static_cast<int>(types[0]));

    const SensorSlotConfiguration& temperatureAndHumidity =
        fixture.service.getConfiguration().sensorSlots[3];
    count = thresholdMeasurementTypesForSensor(
        temperatureAndHumidity, types, MaxImplementationMeasurementTypeCount);
    TEST_ASSERT_EQUAL_UINT32(2, count);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::Temperature),
        static_cast<int>(types[0]));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::RelativeHumidity),
        static_cast<int>(types[1]));
}

void test_threshold_web_bme280_measurements_are_compatible_unique_state_values() {
    Fixture fixture;
    SensorSlotConfiguration bme280 =
        fixture.service.getConfiguration().sensorSlots[0];
    bme280.enabled = true;
    bme280.implementation = SensorImplementation::BME280;
    MeasurementType types[MaxImplementationMeasurementTypeCount];
    const size_t count = thresholdMeasurementTypesForSensor(
        bme280, types, MaxImplementationMeasurementTypeCount);
    TEST_ASSERT_EQUAL_UINT32(3, count);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::Temperature),
        static_cast<int>(types[0]));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::RelativeHumidity),
        static_cast<int>(types[1]));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::AtmosphericPressure),
        static_cast<int>(types[2]));
    for (size_t left = 0; left < count; ++left) {
        TEST_ASSERT_TRUE(isThresholdCompatibleMeasurementType(types[left]));
        for (size_t right = left + 1; right < count; ++right) {
            TEST_ASSERT_NOT_EQUAL(static_cast<int>(types[left]),
                static_cast<int>(types[right]));
        }
    }

    SensorSlotConfiguration eventSensor = fixture.rainGauge();
    TEST_ASSERT_EQUAL_UINT32(0, thresholdMeasurementTypesForSensor(
        eventSensor, types, MaxImplementationMeasurementTypeCount));
    TEST_ASSERT_FALSE(isThresholdCompatibleMeasurementType(
        MeasurementType::RainDetectorWet));
}

void test_threshold_web_sensor_switch_preserves_only_a_supported_measurement() {
    Fixture fixture;
    const SensorSlotConfiguration& temperatureAndHumidity =
        fixture.service.getConfiguration().sensorSlots[3];
    const SensorSlotConfiguration& temperatureOnly =
        fixture.service.getConfiguration().sensorSlots[0];

    TEST_ASSERT_TRUE(sensorSupportsThresholdMeasurement(
        temperatureAndHumidity, MeasurementType::RelativeHumidity));
    TEST_ASSERT_FALSE(sensorSupportsThresholdMeasurement(
        temperatureOnly, MeasurementType::RelativeHumidity));
    TEST_ASSERT_TRUE(sensorSupportsThresholdMeasurement(
        temperatureOnly, MeasurementType::Temperature));
}

void test_threshold_web_fields_map_to_typed_configuration_without_changing_common_or_blink_fields() {
    ControllerSlotConfiguration slot;
    slot.enabled = true;
    slot.name = "Web Threshold";
    slot.implementation = ControllerImplementation::Threshold;
    slot.implementationConfiguration.blink.targetActuatorId = 7;
    slot.implementationConfiguration.blink.onDurationMs = 123;
    slot.implementationConfiguration.blink.offDurationMs = 456;
    TEST_ASSERT_TRUE(applyThresholdControllerWebFields(
        "4", "relative_humidity", "1", "70.5", "64.25", "15000", slot));
    const ThresholdControllerConfiguration& threshold =
        slot.implementationConfiguration.threshold;
    TEST_ASSERT_TRUE(slot.enabled);
    TEST_ASSERT_EQUAL_STRING("Web Threshold", slot.name.c_str());
    TEST_ASSERT_EQUAL_UINT16(4, threshold.source.sensorId);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::RelativeHumidity),
        static_cast<int>(threshold.source.measurementType));
    TEST_ASSERT_EQUAL_UINT16(1, threshold.targetActuatorId);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 70.5F, threshold.onThreshold);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 64.25F, threshold.offThreshold);
    TEST_ASSERT_EQUAL_UINT32(15000, threshold.maxMeasurementAgeMs);
    TEST_ASSERT_EQUAL_UINT16(7,
        slot.implementationConfiguration.blink.targetActuatorId);
    TEST_ASSERT_EQUAL_UINT32(123,
        slot.implementationConfiguration.blink.onDurationMs);
    TEST_ASSERT_EQUAL_UINT32(456,
        slot.implementationConfiguration.blink.offDurationMs);
}

void test_threshold_web_syntax_and_configuration_validation_reject_invalid_posts() {
    Fixture fixture;
    TEST_ASSERT_TRUE(fixture.service.setActuatorSlotConfiguration(
        fixture.enabledActuator()));
    ControllerSlotConfiguration slot = fixture.threshold();
    TEST_ASSERT_FALSE(applyThresholdControllerWebFields(
        "bad", "temperature", "1", "70", "65", "15000", slot));
    TEST_ASSERT_FALSE(applyThresholdControllerWebFields(
        "1", "unknown", "1", "70", "65", "15000", slot));
    TEST_ASSERT_FALSE(applyThresholdControllerWebFields(
        "1", "temperature", "1", "nan", "65", "15000", slot));

    TEST_ASSERT_TRUE(applyThresholdControllerWebFields(
        "2", "temperature", "1", "70", "65", "15000", slot));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
    TEST_ASSERT_TRUE(applyThresholdControllerWebFields(
        "1", "temperature", "1", "65", "70", "15000", slot));
    TEST_ASSERT_FALSE(fixture.service.setControllerSlotConfiguration(slot));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_controller_registry_uses_stable_ids_and_on_off_requirement);
    RUN_TEST(test_threshold_registry_uses_stable_id_and_on_off_requirement);
    RUN_TEST(test_threshold_direction_exposes_matching_comparison_symbols);
    RUN_TEST(test_default_controller_slots_are_disabled_and_stably_identified);
    RUN_TEST(test_valid_blink_targets_enabled_on_off_actuator);
    RUN_TEST(test_on_off_controller_accepts_level_actuator_and_persistence_round_trips);
    RUN_TEST(test_invalid_controller_slot_identity_is_rejected);
    RUN_TEST(test_invalid_or_disabled_or_none_target_is_rejected);
    RUN_TEST(test_incompatible_capability_metadata_is_detected);
    RUN_TEST(test_invalid_blink_durations_are_rejected);
    RUN_TEST(test_enabled_controllers_must_exclusively_own_their_targets);
    RUN_TEST(test_disabled_controller_does_not_claim_target_and_enable_is_revalidated);
    RUN_TEST(test_target_exclusivity_applies_across_controller_implementations);
    RUN_TEST(test_enabled_controllers_may_own_different_targets);
    RUN_TEST(test_controller_web_target_filter_excludes_other_claim_but_keeps_own_claim);
    RUN_TEST(test_referenced_actuator_cannot_be_disabled_or_made_incompatible);
    RUN_TEST(test_disabled_controller_does_not_constrain_actuator_configuration);
    RUN_TEST(test_persistence_uses_stable_id_and_round_trips_blink_parameters);
    RUN_TEST(test_module_target_persists_without_current_runtime_actuator);
    RUN_TEST(test_enabled_controllers_exclusively_claim_same_module_device);
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
    RUN_TEST(test_threshold_web_filters_measurements_sensors_and_actuators_by_metadata);
    RUN_TEST(test_threshold_web_measurements_are_scoped_to_selected_sensor_implementation);
    RUN_TEST(test_threshold_web_bme280_measurements_are_compatible_unique_state_values);
    RUN_TEST(test_threshold_web_sensor_switch_preserves_only_a_supported_measurement);
    RUN_TEST(test_threshold_web_fields_map_to_typed_configuration_without_changing_common_or_blink_fields);
    RUN_TEST(test_threshold_web_syntax_and_configuration_validation_reject_invalid_posts);
    return UNITY_END();
}
