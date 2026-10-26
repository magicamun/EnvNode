// EnvNode: see repository LICENSE.
#include <unity.h>
#include "I2CBusController.h"
#include "I2CBusManager.h"
#include "IdentityEeprom24LC32.h"
#include "BoardProfile.h"
#include <cstring>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t n) { return n; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
class Logger : public ILogger {
public:
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
};
void setUp() { Wire=TwoWire(); Wire1=TwoWire(); arduinoTestMillis()=0; }
void tearDown() {}
void test_fixed_pins_and_bus_selection() {
    Logger logger; I2CBusController buses(logger);
    TEST_ASSERT_NULL(buses.wire(I2CBus::I2C1));
    TEST_ASSERT_TRUE(buses.begin({I2CBus::I2C1,16,5}));
    TEST_ASSERT_EQUAL(16,Wire1.sda);
    TEST_ASSERT_EQUAL(5,Wire1.scl);
    TEST_ASSERT_EQUAL(0,Wire.begins);
    TEST_ASSERT_EQUAL_PTR(&Wire1,buses.wire(I2CBus::I2C1));
    TEST_ASSERT_TRUE(buses.begin({I2CBus::I2C1,16,5}));
    TEST_ASSERT_EQUAL(1,Wire1.begins);
    TEST_ASSERT_FALSE(buses.begin({I2CBus::I2C1,21,22}));
    TEST_ASSERT_EQUAL(16,Wire1.sda);
}
void test_failed_initialization_can_retry_and_invalid_bus_is_rejected() {
    Logger logger; I2CBusController buses(logger); Wire.beginOk=false;
    TEST_ASSERT_FALSE(buses.begin({I2CBus::I2C0,21,22}));
    TEST_ASSERT_NULL(buses.wire(I2CBus::I2C0));
    Wire.beginOk=true;
    TEST_ASSERT_TRUE(buses.begin({I2CBus::I2C0,21,22}));
    TEST_ASSERT_FALSE(buses.begin({static_cast<I2CBus>(2),16,5}));
    TEST_ASSERT_NULL(buses.wire(static_cast<I2CBus>(2)));
}
void test_envnode_bootstrap_and_profile_preserve_wiring() {
    Logger logger; I2CBusManager buses(logger,EnvNodeIdentityI2C);
    TEST_ASSERT_TRUE(buses.beginIdentityBus());
    TEST_ASSERT_EQUAL(21,Wire.sda);
    TEST_ASSERT_EQUAL(22,Wire.scl);
    TEST_ASSERT_TRUE(selectCurrentBoardProfile(BoardProfileId::EnvNodeMini));
    buses.begin();
    TEST_ASSERT_EQUAL(1,Wire.begins);
    TEST_ASSERT_EQUAL(25,Wire1.sda);
    TEST_ASSERT_EQUAL(26,Wire1.scl);
}
void test_eeprom_selected_bus_round_trip_crosses_page_and_read_boundaries() {
    Logger logger; I2CBusController buses(logger);
    buses.begin({I2CBus::I2C1,16,5});
    IdentityEeprom24LC32 eeprom(buses,0x53,I2CBus::I2C1);
    uint8_t source[70], target[70]={};
    for(size_t i=0;i<sizeof(source);++i)source[i]=static_cast<uint8_t>(i);
    TEST_ASSERT_TRUE(eeprom.write(30,source,sizeof(source)));
    const size_t expectedSizes[]={4,34,34,6};
    size_t page=0;
    for(const auto& tx:Wire1.transmissions) {
        if(tx.bytes.empty())continue;
        TEST_ASSERT_EQUAL(0x53,tx.address);
        TEST_ASSERT_LESS_THAN(4,page);
        TEST_ASSERT_EQUAL(expectedSizes[page++],tx.bytes.size());
    }
    TEST_ASSERT_EQUAL(4,page);
    TEST_ASSERT_TRUE(eeprom.read(30,target,sizeof(target)));
    TEST_ASSERT_EQUAL_MEMORY(source,target,sizeof(source));
    TEST_ASSERT_EQUAL(3,Wire1.requests);
    TEST_ASSERT_EQUAL(0,Wire.transmissions.size());
    TEST_ASSERT_FALSE(Wire1.transmissions[8].stop);
}
void test_eeprom_defaults_to_envnode_bus_and_isolates_device_addresses() {
    Logger logger; I2CBusController buses(logger); buses.begin(EnvNodeIdentityI2C);
    IdentityEeprom24LC32 first(buses,0x50),second(buses,0x52);
    const uint8_t source[]={1,2,3};uint8_t target[3]={};
    TEST_ASSERT_TRUE(first.write(0,source,sizeof(source)));
    TEST_ASSERT_TRUE(second.read(0,target,sizeof(target)));
    TEST_ASSERT_EQUAL(0xff,target[0]);
    TEST_ASSERT_TRUE(first.read(0,target,sizeof(target)));
    TEST_ASSERT_EQUAL_MEMORY(source,target,sizeof(source));
    TEST_ASSERT_EQUAL(0,Wire1.transmissions.size());
}
void test_eeprom_bounds_and_unavailable_bus_do_not_transmit() {
    Logger logger; I2CBusController buses(logger);
    IdentityEeprom24LC32 eeprom(buses,0x50,I2CBus::I2C1);uint8_t bytes[2]={};
    TEST_ASSERT_FALSE(eeprom.read(0,bytes,2));
    TEST_ASSERT_FALSE(eeprom.write(0,bytes,2));
    buses.begin({I2CBus::I2C1,16,5});
    TEST_ASSERT_FALSE(eeprom.read(4095,bytes,2));
    TEST_ASSERT_FALSE(eeprom.write(4095,bytes,2));
    TEST_ASSERT_FALSE(eeprom.read(0,nullptr,2));
    TEST_ASSERT_FALSE(eeprom.write(0,bytes,0));
    TEST_ASSERT_EQUAL(0,Wire1.transmissions.size());
    TEST_ASSERT_TRUE(eeprom.write(4095,bytes,1));
    TEST_ASSERT_TRUE(eeprom.read(4095,bytes,1));
}
void test_read_shortfall_and_write_shortfall_are_reported() {
    Logger logger; I2CBusController buses(logger);buses.begin(EnvNodeIdentityI2C);
    IdentityEeprom24LC32 eeprom(buses,0x50);uint8_t bytes[4]={};
    Wire.readLimit=3;
    TEST_ASSERT_FALSE(eeprom.read(0,bytes,4));
    Wire.writeLimit=1;
    TEST_ASSERT_FALSE(eeprom.write(0,bytes,4));
}
void test_nack_and_ready_timeout_are_reported() {
    Logger logger; I2CBusController buses(logger);buses.begin(EnvNodeIdentityI2C);
    IdentityEeprom24LC32 eeprom(buses,0x50);uint8_t byte=1;
    Wire.defaultResult=2;
    TEST_ASSERT_FALSE(eeprom.read(0,&byte,1));
    TEST_ASSERT_FALSE(eeprom.write(0,&byte,1));
    Wire.endResults.push_back(0);
    TEST_ASSERT_FALSE(eeprom.write(0,&byte,1));
    TEST_ASSERT_EQUAL(10,millis());
}
void test_scan_reports_selected_bus_addresses_and_probe_errors() {
    Logger logger; I2CBusController buses(logger);I2CScanResult result;
    buses.scan(I2CBus::I2C1,result);
    TEST_ASSERT_EQUAL(static_cast<int>(I2CScanStatus::BusUnavailable),static_cast<int>(result.status));
    buses.begin({I2CBus::I2C1,16,5});Wire1.defaultResult=2;
    Wire1.results={{0x50,0},{0x53,0},{0x60,4}};
    buses.scan(I2CBus::I2C1,result);
    TEST_ASSERT_EQUAL(2,result.addressCount);
    TEST_ASSERT_EQUAL(0x50,result.addresses[0]);
    TEST_ASSERT_EQUAL(0x53,result.addresses[1]);
    TEST_ASSERT_EQUAL(1,result.probeErrorCount);
    TEST_ASSERT_EQUAL(static_cast<int>(I2CScanStatus::CompleteWithProbeErrors),static_cast<int>(result.status));
    TEST_ASSERT_NOT_NULL(buses.lastScan(I2CBus::I2C1));
    TEST_ASSERT_EQUAL(2,buses.lastScan(I2CBus::I2C1)->addressCount);
    TEST_ASSERT_EQUAL(0,Wire.transmissions.size());
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_fixed_pins_and_bus_selection);
    RUN_TEST(test_failed_initialization_can_retry_and_invalid_bus_is_rejected);
    RUN_TEST(test_envnode_bootstrap_and_profile_preserve_wiring);
    RUN_TEST(test_eeprom_selected_bus_round_trip_crosses_page_and_read_boundaries);
    RUN_TEST(test_eeprom_defaults_to_envnode_bus_and_isolates_device_addresses);
    RUN_TEST(test_eeprom_bounds_and_unavailable_bus_do_not_transmit);
    RUN_TEST(test_read_shortfall_and_write_shortfall_are_reported);
    RUN_TEST(test_nack_and_ready_timeout_are_reported);
    RUN_TEST(test_scan_reports_selected_bus_addresses_and_probe_errors);
    return UNITY_END();
}
