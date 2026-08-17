#include <unity.h>

#include "HardwareResources.h"
#include "IOnOffActuator.h"
#include "ActuatorImplementationRegistry.h"
#include "ActuatorFactory.h"
#include "ActuatorRuntime.h"

using namespace EnvNode;

void pinMode(unsigned char, int) {
}

void digitalWrite(unsigned char, int) {
}

class TestLogger : public ILogger {
public:
    void begin(unsigned long) override {
    }

    void println(const char*) override {
    }

    void printf(const char*, ...) override {
    }
};

void initializeActuatorSlots(ActuatorSlotConfiguration* slots) {
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        slots[index].slotId = static_cast<ActuatorId>(index + 1);
        slots[index].enabled = false;
        slots[index].name = "Unused";
        slots[index].implementation = ActuatorImplementation::None;
        slots[index].hardware = HardwareResourceAssignment::none();
    }
}

void configureGpioOnOffSlot(
    ActuatorSlotConfiguration& slot,
    const char* name,
    uint8_t gpio) {
    slot.enabled = true;
    slot.name = name;
    slot.implementation = ActuatorImplementation::GpioOnOff;
    slot.hardware = HardwareResourceAssignment::gpioResource(GpioResource(gpio));
}

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

void test_runtime_constructs_independent_gpio_on_off_instances_and_looks_up_by_id() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    configureGpioOnOffSlot(slots[0], "First", 16);
    configureGpioOnOffSlot(slots[1], "Second", 17);

    runtime.initialize(slots);

    IOnOffActuator* first = runtime.onOffActuator(1);
    IOnOffActuator* second = runtime.onOffActuator(2);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_NOT_EQUAL(first, second);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ActuatorOperationResult::Completed),
        static_cast<int>(first->setState(OnOffState::On)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On), static_cast<int>(first->state()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off), static_cast<int>(second->state()));
    TEST_ASSERT_EQUAL_UINT32(2, runtime.runtimeCount());
    TEST_ASSERT_EQUAL_UINT32(2, runtime.availableCount());
}

void test_runtime_lookup_fails_for_unknown_id() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    configureGpioOnOffSlot(slots[0], "First", 16);

    runtime.initialize(slots);

    TEST_ASSERT_NULL(runtime.onOffActuator(99));
    const ActuatorRuntime& constRuntime = runtime;
    TEST_ASSERT_NULL(constRuntime.onOffActuator(99));
}

void test_disabled_and_none_slots_do_not_create_runtime_actuators() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    configureGpioOnOffSlot(slots[0], "Disabled", 16);
    slots[0].enabled = false;
    slots[1].enabled = true;
    slots[1].name = "None";

    runtime.initialize(slots);

    TEST_ASSERT_EQUAL_UINT32(0, runtime.runtimeCount());
    TEST_ASSERT_NULL(runtime.onOffActuator(1));
    TEST_ASSERT_NULL(runtime.onOffActuator(2));
}

void test_invalid_hardware_is_unavailable_without_blocking_valid_slot() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    configureGpioOnOffSlot(slots[0], "Invalid", 34);
    configureGpioOnOffSlot(slots[1], "Valid", 17);

    runtime.initialize(slots);

    TEST_ASSERT_NULL(runtime.onOffActuator(1));
    IOnOffActuator* valid = runtime.onOffActuator(2);
    TEST_ASSERT_NOT_NULL(valid);
    TEST_ASSERT_TRUE(valid->initialized());
    TEST_ASSERT_EQUAL_UINT32(2, runtime.runtimeCount());
    TEST_ASSERT_EQUAL_UINT32(1, runtime.availableCount());

    ActuatorRuntimeInfo invalidInfo;
    TEST_ASSERT_TRUE(runtime.runtimeInfo(0, invalidInfo));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ActuatorFactoryResult::InvalidResource),
        static_cast<int>(invalidInfo.constructionResult));
    TEST_ASSERT_FALSE(invalidInfo.initializationAttempted);
    TEST_ASSERT_FALSE(invalidInfo.available);
}

void test_capability_lookup_exposes_on_off_interface() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    configureGpioOnOffSlot(slots[0], "Switch", 16);

    runtime.initialize(slots);

    IOnOffActuator* capability = runtime.onOffActuator(1);
    TEST_ASSERT_NOT_NULL(capability);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off), static_cast<int>(capability->state()));
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
    RUN_TEST(test_runtime_constructs_independent_gpio_on_off_instances_and_looks_up_by_id);
    RUN_TEST(test_runtime_lookup_fails_for_unknown_id);
    RUN_TEST(test_disabled_and_none_slots_do_not_create_runtime_actuators);
    RUN_TEST(test_invalid_hardware_is_unavailable_without_blocking_valid_slot);
    RUN_TEST(test_capability_lookup_exposes_on_off_interface);
    return UNITY_END();
}
