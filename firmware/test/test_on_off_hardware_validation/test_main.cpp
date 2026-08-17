#include <unity.h>

#include "HardwareResources.h"
#include "IOnOffActuator.h"
#include "ActuatorImplementationRegistry.h"

using namespace EnvNode;

void test_gpio_with_digital_output_is_accepted() {
    const HardwareResourceAssignment assignment =
        HardwareResourceAssignment::gpioResource(GpioResource(4));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareResourceValidationResult::Valid),
        static_cast<int>(BoardCapabilities::current().validate(
            HardwareInterfaceKind::GPIO,
            assignment,
            GpioCapability::DigitalOutput)));
}

void test_gpio_without_digital_output_is_rejected() {
    const HardwareResourceAssignment assignment =
        HardwareResourceAssignment::gpioResource(GpioResource(34));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareResourceValidationResult::ResourceUnavailable),
        static_cast<int>(BoardCapabilities::current().validate(
            HardwareInterfaceKind::GPIO,
            assignment,
            GpioCapability::DigitalOutput)));
}

void test_nonexistent_gpio_is_rejected() {
    const HardwareResourceAssignment assignment =
        HardwareResourceAssignment::gpioResource(GpioResource(99));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareResourceValidationResult::ResourceDoesNotExist),
        static_cast<int>(BoardCapabilities::current().validate(
            HardwareInterfaceKind::GPIO,
            assignment,
            GpioCapability::DigitalOutput)));
}

void test_on_off_state_has_stable_binary_values() {
    TEST_ASSERT_EQUAL_UINT8(0, static_cast<uint8_t>(OnOffState::Off));
    TEST_ASSERT_EQUAL_UINT8(1, static_cast<uint8_t>(OnOffState::On));
}

void test_gpio_on_off_registry_metadata_is_stable() {
    const ActuatorImplementationMetadata* metadata =
        ActuatorImplementationRegistry::findByStableId("gpio_on_off");

    TEST_ASSERT_NOT_NULL(metadata);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ActuatorImplementation::GpioOnOff),
        static_cast<int>(metadata->implementation));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ActuatorCapability::OnOff),
        static_cast<int>(metadata->capabilities));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareInterfaceKind::GPIO),
        static_cast<int>(metadata->interfaceKind));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(GpioCapability::DigitalOutput),
        static_cast<int>(metadata->requiredGpioCapabilities));
}

void test_multiple_gpio_on_off_slots_on_different_gpios_are_valid() {
    const HardwareResourceClaim claims[] = {
        {true, HardwareResourceAssignment::gpioResource(GpioResource(16))},
        {true, HardwareResourceAssignment::gpioResource(GpioResource(17))},
        {true, HardwareResourceAssignment::gpioResource(GpioResource(18))},
    };

    TEST_ASSERT_TRUE(validateExclusiveHardwareResourceOccupancy(
        claims, sizeof(claims) / sizeof(claims[0])));
}

void test_duplicate_actuator_gpio_is_rejected() {
    const HardwareResourceClaim claims[] = {
        {true, HardwareResourceAssignment::gpioResource(GpioResource(16))},
        {true, HardwareResourceAssignment::gpioResource(GpioResource(16))},
    };

    TEST_ASSERT_FALSE(validateExclusiveHardwareResourceOccupancy(
        claims, sizeof(claims) / sizeof(claims[0])));
}

void test_sensor_actuator_gpio_collision_is_rejected() {
    const HardwareResourceClaim sensorAndActuatorClaims[] = {
        {true, HardwareResourceAssignment::gpioResource(GpioResource(4))},
        {true, HardwareResourceAssignment::gpioResource(GpioResource(4))},
    };

    TEST_ASSERT_FALSE(validateExclusiveHardwareResourceOccupancy(
        sensorAndActuatorClaims,
        sizeof(sensorAndActuatorClaims) / sizeof(sensorAndActuatorClaims[0])));
}

void test_disabled_actuator_slot_does_not_claim_hardware() {
    const HardwareResourceClaim claims[] = {
        {true, HardwareResourceAssignment::gpioResource(GpioResource(16))},
        {false, HardwareResourceAssignment::gpioResource(GpioResource(16))},
    };

    TEST_ASSERT_TRUE(validateExclusiveHardwareResourceOccupancy(
        claims, sizeof(claims) / sizeof(claims[0])));
}

void test_i2c_devices_on_same_bus_with_different_addresses_can_share_bus() {
    const HardwareResourceClaim claims[] = {
        {true, HardwareResourceAssignment::i2cResource(I2CResource(I2CBus::I2C0, 0x44))},
        {true, HardwareResourceAssignment::i2cResource(I2CResource(I2CBus::I2C0, 0x76))},
    };

    TEST_ASSERT_TRUE(validateExclusiveHardwareResourceOccupancy(
        claims, sizeof(claims) / sizeof(claims[0])));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_gpio_with_digital_output_is_accepted);
    RUN_TEST(test_gpio_without_digital_output_is_rejected);
    RUN_TEST(test_nonexistent_gpio_is_rejected);
    RUN_TEST(test_on_off_state_has_stable_binary_values);
    RUN_TEST(test_gpio_on_off_registry_metadata_is_stable);
    RUN_TEST(test_multiple_gpio_on_off_slots_on_different_gpios_are_valid);
    RUN_TEST(test_duplicate_actuator_gpio_is_rejected);
    RUN_TEST(test_sensor_actuator_gpio_collision_is_rejected);
    RUN_TEST(test_disabled_actuator_slot_does_not_claim_hardware);
    RUN_TEST(test_i2c_devices_on_same_bus_with_different_addresses_can_share_bus);
    return UNITY_END();
}
