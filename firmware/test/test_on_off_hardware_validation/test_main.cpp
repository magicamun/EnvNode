#include <unity.h>

#include "HardwareResources.h"
#include "BoardProfile.h"
#include "IOnOffActuator.h"
#include "ActuatorImplementationRegistry.h"
#include "ActuatorFactory.h"
#include "ActuatorRuntime.h"
#include "ConfigurationRuntimeEffect.h"
#include "SerialLogger.h"
#include "StructuredLogger.h"
#include "RecentLogStore.h"
#include "ILogTimeProvider.h"

using namespace EnvNode;

HardwareSerial Serial;
std::string serialOutput;
size_t serialWriteCallCount = 0;
bool serialBegan = false;
int gpioModes[256] = {};
int gpioValues[256] = {};
int gpioEventPins[512] = {};
int gpioEventActions[512] = {};
size_t gpioEventCount = 0;

void HardwareSerial::begin(unsigned long) {
    serialBegan = true;
}

void HardwareSerial::println(const char* value) {
    serialOutput += value == nullptr ? "" : value;
    serialOutput += '\n';
}

size_t HardwareSerial::write(const uint8_t* data, size_t length) {
    ++serialWriteCallCount;
    serialOutput.append(reinterpret_cast<const char*>(data), length);
    return length;
}

void pinMode(unsigned char pin, int mode) {
    gpioModes[pin] = mode;
    gpioEventPins[gpioEventCount] = pin;
    gpioEventActions[gpioEventCount++] = 100 + mode;
}

void digitalWrite(unsigned char pin, int value) {
    gpioValues[pin] = value;
    gpioEventPins[gpioEventCount] = pin;
    gpioEventActions[gpioEventCount++] = value;
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

class TestLogTimeProvider : public ILogTimeProvider {
public:
    uint32_t monotonicMs() const override { return 0; }
    bool wallClockEpochSeconds(int64_t&) const override { return false; }
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

void configureGpioPwmSlot(
    ActuatorSlotConfiguration& slot,
    const char* name,
    uint8_t gpio) {
    slot.enabled = true;
    slot.name = name;
    slot.implementation = ActuatorImplementation::GpioPwm;
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

void test_current_board_profile_describes_envnode_mainboard() {
    const BoardProfile& profile = currentBoardProfile();

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardProfileId::EnvNodeMainboard),
        static_cast<int>(profile.id));
    TEST_ASSERT_EQUAL_STRING("EnvNode Mainboard", profile.displayName);
    TEST_ASSERT_EQUAL_UINT8(0, profile.revision.major);
    TEST_ASSERT_EQUAL_UINT8(3, profile.revision.minor);
    TEST_ASSERT_EQUAL_UINT32(15, profile.gpioCount);
    TEST_ASSERT_EQUAL_UINT32(2, profile.i2cBusCount);
    TEST_ASSERT_EQUAL_UINT32(1, boardProfileCount());
    TEST_ASSERT_EQUAL_PTR(&profile, boardProfileAt(0));
    TEST_ASSERT_EQUAL_PTR(&profile, boardProfile(BoardProfileId::EnvNodeMainboard));
    TEST_ASSERT_NULL(boardProfileAt(1));
}

void test_current_board_profile_has_exact_gpio_and_i2c_mapping() {
    const BoardProfile& profile = currentBoardProfile();
    const uint8_t expectedGpios[] = {
        4, 13, 14, 15, 16, 17, 18, 19, 23, 32, 33, 34, 35, 36, 39,
    };

    TEST_ASSERT_EQUAL_UINT32(
        sizeof(expectedGpios) / sizeof(expectedGpios[0]), profile.gpioCount);
    for (size_t index = 0; index < profile.gpioCount; ++index) {
        TEST_ASSERT_EQUAL_UINT8(expectedGpios[index], profile.gpios[index].resource.number);
    }
    TEST_ASSERT_EQUAL_INT(static_cast<int>(I2CBus::I2C0),
        static_cast<int>(profile.i2cBuses[0].bus));
    TEST_ASSERT_EQUAL_UINT8(21, profile.i2cBuses[0].sda.number);
    TEST_ASSERT_EQUAL_UINT8(22, profile.i2cBuses[0].scl.number);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(I2CBus::I2C1),
        static_cast<int>(profile.i2cBuses[1].bus));
    TEST_ASSERT_EQUAL_UINT8(25, profile.i2cBuses[1].sda.number);
    TEST_ASSERT_EQUAL_UINT8(26, profile.i2cBuses[1].scl.number);

    TEST_ASSERT_NULL(BoardCapabilities::current().gpio(GpioResource(21)));
    TEST_ASSERT_NULL(BoardCapabilities::current().gpio(GpioResource(22)));
    TEST_ASSERT_NULL(BoardCapabilities::current().gpio(GpioResource(25)));
    TEST_ASSERT_NULL(BoardCapabilities::current().gpio(GpioResource(26)));
}

void test_current_board_profile_preserves_analog_capabilities() {
    const BoardCapabilities& capabilities = BoardCapabilities::current();
    const uint8_t analogGpios[] = {32, 33, 34, 35, 36, 39};
    for (size_t index = 0; index < sizeof(analogGpios) / sizeof(analogGpios[0]); ++index) {
        const BoardGpioCapability* gpio = capabilities.gpio(GpioResource(analogGpios[index]));
        TEST_ASSERT_NOT_NULL(gpio);
        TEST_ASSERT_TRUE(hasGpioCapabilities(
            gpio->capabilities, GpioCapability::AnalogInput));
    }
    TEST_ASSERT_TRUE(hasGpioCapabilities(
        capabilities.gpio(GpioResource(32))->capabilities,
        GpioCapability::DigitalOutput));
    TEST_ASSERT_FALSE(hasGpioCapabilities(
        capabilities.gpio(GpioResource(34))->capabilities,
        GpioCapability::DigitalOutput));
}

void test_on_off_state_has_stable_binary_values() {
    TEST_ASSERT_EQUAL_UINT8(0, static_cast<uint8_t>(OnOffState::Off));
    TEST_ASSERT_EQUAL_UINT8(1, static_cast<uint8_t>(OnOffState::On));
}

void test_actuator_level_validation_and_pwm_mapping() {
    const uint8_t validPercentages[] = {0, 1, 50, 99, 100};
    for (size_t index = 0; index < sizeof(validPercentages); ++index) {
        ActuatorLevel level;
        TEST_ASSERT_TRUE(ActuatorLevel::tryCreate(validPercentages[index], level));
        TEST_ASSERT_EQUAL_UINT8(validPercentages[index], level.percent());
    }
    ActuatorLevel invalid;
    TEST_ASSERT_FALSE(ActuatorLevel::tryCreate(101, invalid));
    TEST_ASSERT_EQUAL_UINT32(0, gpioPwmDutyForLevel(ActuatorLevel::off()));
    ActuatorLevel half;
    TEST_ASSERT_TRUE(ActuatorLevel::tryCreate(50, half));
    TEST_ASSERT_EQUAL_UINT32(128, gpioPwmDutyForLevel(half));
    TEST_ASSERT_EQUAL_UINT32(255, gpioPwmDutyForLevel(ActuatorLevel::full()));
}

void test_level_capability_centrally_satisfies_on_off_but_not_inverse() {
    TEST_ASSERT_TRUE(hasActuatorCapability(
        ActuatorCapability::OnOff, ActuatorCapability::OnOff));
    TEST_ASSERT_FALSE(hasActuatorCapability(
        ActuatorCapability::OnOff, ActuatorCapability::Level));
    TEST_ASSERT_TRUE(hasActuatorCapability(
        ActuatorCapability::Level, ActuatorCapability::OnOff));
    TEST_ASSERT_TRUE(hasActuatorCapability(
        ActuatorCapability::Level, ActuatorCapability::Level));
}

void test_gpio_pwm_level_on_off_mapping_does_not_restore_previous_level() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    configureGpioPwmSlot(slots[0], "Level", 16);
    runtime.initialize(slots);

    ILevelActuator* actuator = runtime.levelActuator(1);
    TEST_ASSERT_NOT_NULL(actuator);
    TEST_ASSERT_NOT_NULL(runtime.onOffActuator(1));
    TEST_ASSERT_EQUAL_UINT8(0, actuator->level().percent());
    ActuatorLevel partial;
    TEST_ASSERT_TRUE(ActuatorLevel::tryCreate(37, partial));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ActuatorOperationResult::Completed),
        static_cast<int>(actuator->setLevel(partial)));
    TEST_ASSERT_EQUAL_UINT8(37, actuator->level().percent());
    actuator->setState(OnOffState::Off);
    TEST_ASSERT_EQUAL_UINT8(0, actuator->level().percent());
    actuator->setState(OnOffState::On);
    TEST_ASSERT_EQUAL_UINT8(100, actuator->level().percent());
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
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ActuatorOperationResult::Completed),
        static_cast<int>(second->setState(OnOffState::On)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ActuatorOperationResult::Completed),
        static_cast<int>(first->setState(OnOffState::Off)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off), static_cast<int>(first->state()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On), static_cast<int>(second->state()));
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

void test_descriptor_actuator_replaces_saved_actuator_on_same_gpio() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    configureGpioOnOffSlot(slots[4], "Saved relay", 16);
    AutomaticActuatorDefinition automatic;
    automatic.moduleSlot = ModuleSlot::A;
    strcpy(automatic.deviceId, "relay.1");
    strcpy(automatic.name, "A relay.1");
    automatic.implementation = ActuatorImplementation::GpioOnOff;
    automatic.hardware = HardwareResourceAssignment::gpioResource(GpioResource(16));

    TEST_ASSERT_TRUE(runtime.configureAutomaticActuators(&automatic, 1));
    runtime.initialize(slots);

    TEST_ASSERT_EQUAL_UINT32(1, runtime.runtimeCount());
    TEST_ASSERT_NOT_NULL(runtime.onOffActuator(5));
    ActuatorRuntimeInfo info;
    TEST_ASSERT_TRUE(runtime.runtimeInfo(0, info));
    TEST_ASSERT_EQUAL_UINT8(5, info.id);
    TEST_ASSERT_EQUAL_STRING("A relay.1", info.name);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ActuatorRuntimeOrigin::ModuleDescriptor),
        static_cast<int>(info.origin));
    TEST_ASSERT_EQUAL_STRING("relay.1", info.descriptorDeviceId);
}

void test_descriptor_actuator_uses_first_empty_runtime_slot() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    configureGpioOnOffSlot(slots[0], "Saved relay", 16);
    AutomaticActuatorDefinition automatic;
    automatic.moduleSlot = ModuleSlot::B;
    strcpy(automatic.deviceId, "relay.2");
    strcpy(automatic.name, "B relay.2");
    automatic.implementation = ActuatorImplementation::GpioOnOff;
    automatic.hardware = HardwareResourceAssignment::gpioResource(GpioResource(17));

    TEST_ASSERT_TRUE(runtime.configureAutomaticActuators(&automatic, 1));
    runtime.initialize(slots);

    TEST_ASSERT_EQUAL_UINT32(2, runtime.runtimeCount());
    TEST_ASSERT_NOT_NULL(runtime.onOffActuator(1));
    TEST_ASSERT_NOT_NULL(runtime.onOffActuator(2));
    ActuatorRuntimeInfo automaticInfo;
    TEST_ASSERT_TRUE(runtime.runtimeInfo(1, automaticInfo));
    TEST_ASSERT_EQUAL_UINT8(2, automaticInfo.id);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ActuatorRuntimeOrigin::ModuleDescriptor),
        static_cast<int>(automaticInfo.origin));
}

void test_descriptor_actuator_resolves_by_module_uuid_and_device_id() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    AutomaticActuatorDefinition automatic;
    automatic.moduleSlot = ModuleSlot::B;
    automatic.hasModuleInstanceId = true;
    for (size_t index = 0; index < sizeof(automatic.moduleInstanceFingerprint); ++index) {
        automatic.moduleInstanceFingerprint[index] = static_cast<uint8_t>(index + 1);
    }
    strcpy(automatic.deviceId, "relay.2");
    strcpy(automatic.name, "B relay.2");
    automatic.implementation = ActuatorImplementation::GpioOnOff;
    automatic.hardware = HardwareResourceAssignment::gpioResource(GpioResource(17));
    TEST_ASSERT_TRUE(runtime.configureAutomaticActuators(&automatic, 1));
    runtime.initialize(slots);

    ModuleActuatorReference reference;
    setModuleActuatorInstanceId(reference, automatic.moduleInstanceFingerprint);
    setModuleActuatorDeviceId(reference, "relay.2");
    TEST_ASSERT_NOT_NULL(runtime.onOffActuator(reference));
    setModuleActuatorDeviceId(reference, "relay.1");
    TEST_ASSERT_NULL(runtime.onOffActuator(reference));
    reference.moduleInstanceFingerprint[0] ^= 0xff;
    setModuleActuatorDeviceId(reference, "relay.2");
    TEST_ASSERT_NULL(runtime.onOffActuator(reference));
}

void test_serial_logger_preserves_messages_longer_than_old_buffer() {
    RecentLogStore store;
    TestLogTimeProvider timeProvider;
    SerialLogger sink;
    StructuredLogger logger(store, timeProvider, sink);
    const std::string longValue(180, 'x');
    serialOutput.clear();
    logger.begin(115200);

    logger.printf("prefix:%s:suffix\n", longValue.c_str());

    TEST_ASSERT_EQUAL_STRING(
        (std::string("+00:00:00.000 INFO  #1 prefix:") + longValue + ":suffix\r\n").c_str(),
        serialOutput.c_str());
}

void test_serial_logger_println_writes_exact_text_bytes() {
    RecentLogStore store;
    TestLogTimeProvider timeProvider;
    SerialLogger sink;
    StructuredLogger logger(store, timeProvider, sink);
    serialOutput.clear();
    serialWriteCallCount = 0;
    logger.begin(115200);

    logger.println("Actuator runtime rebuild started");

    TEST_ASSERT_EQUAL_STRING(
        "+00:00:00.000 INFO  #1 Actuator runtime rebuild started\r\n",
        serialOutput.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, serialWriteCallCount);
}

void test_serial_logger_printf_uses_one_explicit_length_write() {
    RecentLogStore store;
    TestLogTimeProvider timeProvider;
    SerialLogger sink;
    StructuredLogger logger(store, timeProvider, sink);
    serialOutput.clear();
    serialWriteCallCount = 0;
    logger.begin(115200);

    logger.printf("value=%u\n", 17U);

    TEST_ASSERT_EQUAL_STRING(
        "+00:00:00.000 INFO  #1 value=17\r\n", serialOutput.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, serialWriteCallCount);
}

void test_actuator_configuration_requires_runtime_apply() {
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(RuntimeAction::RestartActuatorRuntime),
        static_cast<int>(runtimeActionFor(ConfigurationArea::Actuators)));
}

void test_controller_configuration_requires_dedicated_runtime_apply() {
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(RuntimeAction::RestartControllerRuntime),
        static_cast<int>(runtimeActionFor(ConfigurationArea::Controllers)));
}

void test_runtime_rebuild_adds_an_actuator_initialized_off() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    runtime.initialize(slots);

    configureGpioOnOffSlot(slots[0], "Added", 16);
    TEST_ASSERT_TRUE(runtime.rebuild(slots));

    IOnOffActuator* actuator = runtime.onOffActuator(1);
    TEST_ASSERT_NOT_NULL(actuator);
    TEST_ASSERT_TRUE(actuator->initialized());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off), static_cast<int>(actuator->state()));
    TEST_ASSERT_EQUAL_INT(OUTPUT, gpioModes[16]);
    TEST_ASSERT_EQUAL_INT(LOW, gpioValues[16]);
}

void test_runtime_rebuild_moves_actuator_after_releasing_old_gpio() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    configureGpioOnOffSlot(slots[0], "Moved", 16);
    runtime.initialize(slots);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ActuatorOperationResult::Completed),
        static_cast<int>(runtime.onOffActuator(1)->setState(OnOffState::On)));

    slots[0].hardware = HardwareResourceAssignment::gpioResource(GpioResource(17));
    gpioEventCount = 0;
    TEST_ASSERT_TRUE(runtime.rebuild(slots));

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(4, gpioEventCount);
    TEST_ASSERT_EQUAL_INT(16, gpioEventPins[0]);
    TEST_ASSERT_EQUAL_INT(LOW, gpioEventActions[0]);
    TEST_ASSERT_EQUAL_INT(16, gpioEventPins[1]);
    TEST_ASSERT_EQUAL_INT(100 + INPUT, gpioEventActions[1]);
    TEST_ASSERT_EQUAL_INT(17, gpioEventPins[2]);
    TEST_ASSERT_EQUAL_INT(100 + OUTPUT, gpioEventActions[2]);
    TEST_ASSERT_EQUAL_INT(LOW, gpioValues[16]);
    TEST_ASSERT_EQUAL_INT(INPUT, gpioModes[16]);
    TEST_ASSERT_EQUAL_INT(OUTPUT, gpioModes[17]);
    IOnOffActuator* moved = runtime.onOffActuator(1);
    TEST_ASSERT_NOT_NULL(moved);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::Off), static_cast<int>(moved->state()));
}

void test_runtime_rebuild_removes_and_deinitializes_actuator() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    configureGpioOnOffSlot(slots[0], "Removed", 16);
    runtime.initialize(slots);
    runtime.onOffActuator(1)->setState(OnOffState::On);

    slots[0].enabled = false;
    TEST_ASSERT_TRUE(runtime.rebuild(slots));

    TEST_ASSERT_NULL(runtime.onOffActuator(1));
    TEST_ASSERT_EQUAL_UINT32(0, runtime.runtimeCount());
    TEST_ASSERT_EQUAL_INT(LOW, gpioValues[16]);
    TEST_ASSERT_EQUAL_INT(INPUT, gpioModes[16]);
}

void test_failed_rebuild_does_not_activate_invalid_new_composition() {
    TestLogger logger;
    ActuatorFactory factory(logger);
    ActuatorRuntime runtime(factory, logger);
    ActuatorSlotConfiguration slots[MaxActuatorSlotCount];
    initializeActuatorSlots(slots);
    configureGpioOnOffSlot(slots[0], "Existing", 16);
    runtime.initialize(slots);
    runtime.onOffActuator(1)->setState(OnOffState::On);

    slots[0].hardware = HardwareResourceAssignment::gpioResource(GpioResource(34));
    TEST_ASSERT_FALSE(runtime.rebuild(slots));

    IOnOffActuator* existing = runtime.onOffActuator(1);
    TEST_ASSERT_NOT_NULL(existing);
    TEST_ASSERT_TRUE(existing->initialized());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OnOffState::On), static_cast<int>(existing->state()));
    TEST_ASSERT_EQUAL_INT(OUTPUT, gpioModes[16]);
    TEST_ASSERT_NOT_EQUAL(OUTPUT, gpioModes[34]);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_gpio_with_digital_output_is_accepted);
    RUN_TEST(test_gpio_without_digital_output_is_rejected);
    RUN_TEST(test_nonexistent_gpio_is_rejected);
    RUN_TEST(test_current_board_profile_describes_envnode_mainboard);
    RUN_TEST(test_current_board_profile_has_exact_gpio_and_i2c_mapping);
    RUN_TEST(test_current_board_profile_preserves_analog_capabilities);
    RUN_TEST(test_on_off_state_has_stable_binary_values);
    RUN_TEST(test_actuator_level_validation_and_pwm_mapping);
    RUN_TEST(test_level_capability_centrally_satisfies_on_off_but_not_inverse);
    RUN_TEST(test_gpio_pwm_level_on_off_mapping_does_not_restore_previous_level);
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
    RUN_TEST(test_descriptor_actuator_replaces_saved_actuator_on_same_gpio);
    RUN_TEST(test_descriptor_actuator_uses_first_empty_runtime_slot);
    RUN_TEST(test_descriptor_actuator_resolves_by_module_uuid_and_device_id);
    RUN_TEST(test_serial_logger_preserves_messages_longer_than_old_buffer);
    RUN_TEST(test_serial_logger_println_writes_exact_text_bytes);
    RUN_TEST(test_serial_logger_printf_uses_one_explicit_length_write);
    RUN_TEST(test_actuator_configuration_requires_runtime_apply);
    RUN_TEST(test_controller_configuration_requires_dedicated_runtime_apply);
    RUN_TEST(test_runtime_rebuild_adds_an_actuator_initialized_off);
    RUN_TEST(test_runtime_rebuild_moves_actuator_after_releasing_old_gpio);
    RUN_TEST(test_runtime_rebuild_removes_and_deinitializes_actuator);
    RUN_TEST(test_failed_rebuild_does_not_activate_invalid_new_composition);
    return UNITY_END();
}
