#include <unity.h>

#include <climits>

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

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_controller_registry_uses_stable_ids_and_on_off_requirement);
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
    return UNITY_END();
}
