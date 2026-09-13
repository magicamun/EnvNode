#include <unity.h>

#include "ConfigurationService.h"
#include "SensorImplementationRegistry.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

SensorSlotConfiguration shtc3Slot(ConfigurationService& service, uint8_t address) {
    SensorSlotConfiguration slot = service.getConfiguration().sensorSlots[4];
    const I2CResource resource(I2CBus::I2C0, address);
    slot.enabled = true;
    slot.name = "SHTC3";
    slot.implementation = SensorImplementation::SHTC3;
    slot.schedule = SensorSchedule::periodic(5000);
    slot.hardware = HardwareResourceAssignment::i2cResource(resource);
    slot.implementationConfiguration.shtc3 = SHTC3Configuration(resource);
    return slot;
}
void test_registry_describes_shtc3_measurements_and_interface() {
    const SensorImplementationMetadata* metadata =
        SensorImplementationRegistry::findByStableId("shtc3");
    TEST_ASSERT_NOT_NULL(metadata);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorImplementation::SHTC3),
        static_cast<int>(metadata->implementation));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HardwareInterfaceKind::I2C),
        static_cast<int>(metadata->interfaceKind));
    TEST_ASSERT_EQUAL_UINT32(2, metadata->measurementTypeCount);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::Temperature),
        static_cast<int>(metadata->measurementTypes[0]));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MeasurementType::RelativeHumidity),
        static_cast<int>(metadata->measurementTypes[1]));
}

void test_shtc3_accepts_only_fixed_i2c_address() {
    ConfigurationService service;
    service.loadConfiguration();
    service.resetToDefaults();

    TEST_ASSERT_TRUE(service.setSensorSlotConfiguration(shtc3Slot(service, 0x70)));
    TEST_ASSERT_FALSE(service.setSensorSlotConfiguration(shtc3Slot(service, 0x71)));
}

void test_shtc3_configuration_persists_and_loads() {
    ConfigurationService service;
    service.loadConfiguration();
    service.resetToDefaults();
    TEST_ASSERT_TRUE(service.setSensorSlotConfiguration(shtc3Slot(service, 0x70)));
    TEST_ASSERT_EQUAL_STRING("shtc3", Preferences::storedString("s5_impl").c_str());

    ConfigurationService loadedService;
    loadedService.loadConfiguration();
    const SensorSlotConfiguration& loaded = loadedService.getConfiguration().sensorSlots[4];
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SensorImplementation::SHTC3),
        static_cast<int>(loaded.implementation));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HardwareResourceKind::I2C),
        static_cast<int>(loaded.hardware.kind));
    TEST_ASSERT_EQUAL_UINT8(0x70, loaded.hardware.i2c.address);
    TEST_ASSERT_EQUAL_UINT8(0x70, loaded.implementationConfiguration.shtc3.i2c.address);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_registry_describes_shtc3_measurements_and_interface);
    RUN_TEST(test_shtc3_accepts_only_fixed_i2c_address);
    RUN_TEST(test_shtc3_configuration_persists_and_loads);
    return UNITY_END();
}
