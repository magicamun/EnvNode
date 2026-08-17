#include <unity.h>

#include "HardwareResources.h"
#include "IOnOffActuator.h"

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

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_gpio_with_digital_output_is_accepted);
    RUN_TEST(test_gpio_without_digital_output_is_rejected);
    RUN_TEST(test_nonexistent_gpio_is_rejected);
    RUN_TEST(test_on_off_state_has_stable_binary_values);
    return UNITY_END();
}
