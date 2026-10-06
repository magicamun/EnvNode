#include <unity.h>
#include "ConfigurationService.h"
#include "ActuatorRuntime.h"
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
class TestLogger : public ILogger {
public:
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
};
struct Fixture {
    ConfigurationService config;
    TestLogger logger;
    ActuatorFactory factory{logger};
    ActuatorRuntime runtime{factory, logger};
    AutomaticActuatorDefinition definition;
    Fixture() {
        config.loadConfiguration();
        config.resetToDefaults();
        definition.hasModuleInstanceId = true;
        definition.moduleInstanceFingerprint[0] = 42;
        strcpy(definition.deviceId, "relay.1");
        strcpy(definition.name, "A relay.1");
        definition.implementation = ActuatorImplementation::GpioOnOff;
        definition.hardware = HardwareResourceAssignment::gpioResource(GpioResource(4));
    }
    ActuatorSlotConfiguration module() {
        auto slot = config.getConfiguration().actuatorSlots[0];
        slot.enabled = true;
        slot.name = "Garden relay";
        slot.implementation = ActuatorImplementation::GpioOnOff;
        slot.hardware = HardwareResourceAssignment::none();
        memcpy(slot.moduleTarget.moduleInstanceFingerprint, definition.moduleInstanceFingerprint, 8);
        setModuleActuatorDeviceId(slot.moduleTarget, definition.deviceId);
        return slot;
    }
};
void test_persistence_and_validation() {
    Fixture f;
    auto slot = f.module();
    TEST_ASSERT_TRUE(f.config.setActuatorSlotConfiguration(slot));
    ConfigurationService reloaded;
    reloaded.loadConfiguration();
    const auto& restored = reloaded.getConfiguration().actuatorSlots[0];
    TEST_ASSERT_TRUE(sameModuleActuatorReference(slot.moduleTarget, restored.moduleTarget));
    TEST_ASSERT_EQUAL_STRING("Garden relay", restored.name.c_str());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HardwareResourceKind::None), static_cast<int>(restored.hardware.kind));
    slot.implementation = ActuatorImplementation::GpioPwm;
    TEST_ASSERT_FALSE(f.config.setActuatorSlotConfiguration(slot));
    slot = f.module();
    slot.hardware = f.definition.hardware;
    TEST_ASSERT_FALSE(f.config.setActuatorSlotConfiguration(slot));
    slot = f.module();
    slot.slotId = 2;
    TEST_ASSERT_FALSE(f.config.setActuatorSlotConfiguration(slot));
}
void test_disabled_module_is_not_recreated_and_can_be_enabled() {
    Fixture f;
    auto slot = f.module();
    slot.enabled = false;
    TEST_ASSERT_TRUE(f.config.setActuatorSlotConfiguration(slot));
    TEST_ASSERT_TRUE(f.runtime.configureAutomaticActuators(&f.definition, 1));
    f.runtime.initialize(f.config.getConfiguration().actuatorSlots);
    TEST_ASSERT_EQUAL_UINT(0, f.runtime.runtimeCount());
    TEST_ASSERT_TRUE(f.runtime.moduleOwnsHardware(f.definition.hardware));
    slot.enabled = true;
    TEST_ASSERT_TRUE(f.config.setActuatorSlotConfiguration(slot));
    TEST_ASSERT_TRUE(f.runtime.rebuild(f.config.getConfiguration().actuatorSlots));
    TEST_ASSERT_EQUAL_UINT(1, f.runtime.availableCount());
    ActuatorRuntimeInfo info;
    TEST_ASSERT_TRUE(f.runtime.runtimeInfo(0, info));
    TEST_ASSERT_EQUAL_STRING("Garden relay", info.name);
    TEST_ASSERT_EQUAL_UINT(4, info.hardware.gpio.number);
}
void test_absent_module_does_not_create_gpio_actuator() {
    Fixture f;
    TEST_ASSERT_TRUE(f.config.setActuatorSlotConfiguration(f.module()));
    f.runtime.initialize(f.config.getConfiguration().actuatorSlots);
    TEST_ASSERT_EQUAL_UINT(0, f.runtime.runtimeCount());
}
void test_module_move_uses_descriptor_gpio() {
    Fixture f;
    TEST_ASSERT_TRUE(f.config.setActuatorSlotConfiguration(f.module()));
    f.definition.moduleSlot = ModuleSlot::B;
    f.definition.hardware = HardwareResourceAssignment::gpioResource(GpioResource(14));
    TEST_ASSERT_TRUE(f.runtime.configureAutomaticActuators(&f.definition, 1));
    f.runtime.initialize(f.config.getConfiguration().actuatorSlots);
    ActuatorRuntimeInfo info;
    TEST_ASSERT_TRUE(f.runtime.runtimeInfo(0, info));
    TEST_ASSERT_EQUAL_UINT(1, info.id);
    TEST_ASSERT_EQUAL_UINT(14, info.hardware.gpio.number);
    TEST_ASSERT_EQUAL_STRING("Garden relay", info.name);
}
void test_replacement_module_does_not_inherit_saved_settings() {
    Fixture f;
    TEST_ASSERT_TRUE(f.config.setActuatorSlotConfiguration(f.module()));
    f.definition.moduleInstanceFingerprint[0] = 43;
    TEST_ASSERT_TRUE(f.runtime.configureAutomaticActuators(&f.definition, 1));
    f.runtime.initialize(f.config.getConfiguration().actuatorSlots);
    TEST_ASSERT_NULL(f.runtime.onOffActuator(1));
    ActuatorRuntimeInfo info;
    TEST_ASSERT_TRUE(f.runtime.runtimeInfo(0, info));
    TEST_ASSERT_EQUAL_STRING("A relay.1", info.name);
    TEST_ASSERT_EQUAL_UINT(2, info.id);
}
void test_disabled_module_reserves_only_its_own_channel() {
    Fixture f;
    auto slot = f.module();
    slot.enabled = false;
    TEST_ASSERT_TRUE(f.config.setActuatorSlotConfiguration(slot));
    AutomaticActuatorDefinition definitions[2] = {f.definition, f.definition};
    strcpy(definitions[1].deviceId, "relay.2");
    strcpy(definitions[1].name, "A relay.2");
    definitions[1].hardware = HardwareResourceAssignment::gpioResource(GpioResource(13));
    TEST_ASSERT_TRUE(f.runtime.configureAutomaticActuators(definitions, 2));
    f.runtime.initialize(f.config.getConfiguration().actuatorSlots);
    TEST_ASSERT_NULL(f.runtime.onOffActuator(1));
    TEST_ASSERT_NOT_NULL(f.runtime.onOffActuator(2));
    TEST_ASSERT_EQUAL_UINT(1, f.runtime.availableCount());
}
void test_manual_gpio_cannot_claim_disabled_module_hardware() {
    Fixture f;
    f.definition.hardware = HardwareResourceAssignment::gpioResource(GpioResource(16));
    auto slot = f.module();
    slot.enabled = false;
    TEST_ASSERT_TRUE(f.config.setActuatorSlotConfiguration(slot));
    TEST_ASSERT_TRUE(f.runtime.configureAutomaticActuators(&f.definition, 1));
    f.runtime.initialize(f.config.getConfiguration().actuatorSlots);
    auto manual = f.config.getConfiguration().actuatorSlots[1];
    manual.enabled = true;
    manual.implementation = ActuatorImplementation::GpioPwm;
    manual.hardware = f.definition.hardware;
    TEST_ASSERT_TRUE(f.config.setActuatorSlotConfiguration(manual));
    TEST_ASSERT_FALSE(f.runtime.rebuild(f.config.getConfiguration().actuatorSlots));
    TEST_ASSERT_EQUAL_UINT(0, f.runtime.runtimeCount());
}
void test_reset_defaults_clears_module_binding() {
    Fixture f;
    TEST_ASSERT_TRUE(f.config.setActuatorSlotConfiguration(f.module()));
    f.config.resetToDefaults();
    TEST_ASSERT_FALSE(validModuleActuatorReference(f.config.getConfiguration().actuatorSlots[0].moduleTarget));
    ConfigurationService reloaded;
    reloaded.loadConfiguration();
    TEST_ASSERT_FALSE(validModuleActuatorReference(reloaded.getConfiguration().actuatorSlots[0].moduleTarget));
}
int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_persistence_and_validation);
    RUN_TEST(test_disabled_module_is_not_recreated_and_can_be_enabled);
    RUN_TEST(test_absent_module_does_not_create_gpio_actuator);
    RUN_TEST(test_module_move_uses_descriptor_gpio);
    RUN_TEST(test_replacement_module_does_not_inherit_saved_settings);
    RUN_TEST(test_disabled_module_reserves_only_its_own_channel);
    RUN_TEST(test_manual_gpio_cannot_claim_disabled_module_hardware);
    RUN_TEST(test_reset_defaults_clears_module_binding);
    return UNITY_END();
}
