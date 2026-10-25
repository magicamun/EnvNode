// EnvNode: see repository LICENSE.
#include <unity.h>
#include "ConfigurationService.h"
#include "ConfigurationBackup.h"
#include "ConfigurationTestStorage.h"
#include <vector>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long){}
void HardwareSerial::println(const char*){}
size_t HardwareSerial::write(const uint8_t*,size_t n){return n;}
void pinMode(unsigned char,int){}
void digitalWrite(unsigned char,int){}
void setUp(){clearConfigurationTestStorage();}
void tearDown(){missingTestPartitions().clear();Preferences::failUIntKey()=String();Preferences::failStringWrites()=false;Preferences::failByteReads()=false;}
void test_usb_table_alone_keeps_legacy_settings(){
    Preferences old;old.begin("weather");old.putString("deviceName","Existing");old.putString("wifiSSID","Existing-Lab");old.putString("wifiPassword","existing-secret");old.putUInt("locale",1);
    ConfigurationService service;service.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("Existing",service.getConfiguration().device.name.c_str());
    TEST_ASSERT_EQUAL_STRING("existing-secret",service.getConfiguration().network.wifiPassword.c_str());
    TEST_ASSERT_EQUAL(1,static_cast<unsigned>(service.getLocale()));
    Preferences metadata;metadata.begin("weather_meta",false,"cfg_nvs");TEST_ASSERT_EQUAL(2,metadata.getUInt("active",2));
    TEST_ASSERT_TRUE(service.setDeviceName("Legacy edit"));
    TEST_ASSERT_EQUAL_STRING("Legacy edit",old.getString("deviceName").c_str());
}
void test_confirmed_restore_migrates_to_dedicated_partition_and_survives_reboot(){
    ConfigurationService service;service.loadConfiguration();service.setDeviceName("Before");service.setWifiPassword("old-secret");
    Configuration candidate=service.getConfiguration();candidate.device.name="Restored";candidate.network.wifiPassword="new-secret";candidate.locale.locale=Locale::EnglishUnitedStates;
    TEST_ASSERT_TRUE(service.restoreConfiguration(candidate));
    Preferences old;old.begin("weather");TEST_ASSERT_EQUAL_STRING("Before",old.getString("deviceName").c_str());
    Preferences metadata;metadata.begin("weather_meta",false,"cfg_nvs");TEST_ASSERT_EQUAL(0,metadata.getUInt("active",2));
    Preferences bank;bank.begin("weather0",false,"cfg_nvs");TEST_ASSERT_GREATER_THAN(0,bank.getBytesLength("cfg_snapshot"));
    ConfigurationService rebooted;rebooted.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("Restored",rebooted.getConfiguration().device.name.c_str());
    TEST_ASSERT_EQUAL_STRING("new-secret",rebooted.getConfiguration().network.wifiPassword.c_str());
    TEST_ASSERT_EQUAL(2,static_cast<unsigned>(rebooted.getLocale()));
    TEST_ASSERT_TRUE(rebooted.setDeviceName("Dedicated edit"));
    TEST_ASSERT_EQUAL_STRING("Dedicated edit",bank.getString("deviceName").c_str());
    TEST_ASSERT_EQUAL_STRING("Before",old.getString("deviceName").c_str());
}
void test_failed_migration_preserves_legacy_source_and_next_boot(){
    ConfigurationService service;service.loadConfiguration();service.setDeviceName("Keep legacy");Configuration candidate=service.getConfiguration();candidate.device.name="Rejected";
    Preferences::failUIntKey()="active";TEST_ASSERT_FALSE(service.restoreConfiguration(candidate));Preferences::failUIntKey()=String();
    TEST_ASSERT_EQUAL_STRING("Keep legacy",service.getConfiguration().device.name.c_str());
    ConfigurationService rebooted;rebooted.loadConfiguration();TEST_ASSERT_EQUAL_STRING("Keep legacy",rebooted.getConfiguration().device.name.c_str());
    Preferences meta;meta.begin("weather_meta",false,"cfg_nvs");TEST_ASSERT_EQUAL(2,meta.getUInt("active",2));
}
void test_missing_partition_keeps_normal_operation_but_does_not_write_a_restore(){
    missingTestPartitions().insert("cfg_nvs");ConfigurationService service;service.loadConfiguration();TEST_ASSERT_TRUE(service.setDeviceName("OTA only"));
    Configuration candidate=service.getConfiguration();candidate.device.name="No partition";
    TEST_ASSERT_FALSE(service.restoreConfiguration(candidate));
    ConfigurationService rebooted;rebooted.loadConfiguration();TEST_ASSERT_EQUAL_STRING("OTA only",rebooted.getConfiguration().device.name.c_str());
}
void test_existing_default_nvs_bank_is_loaded_before_first_dedicated_restore(){
    const ConfigurationStorageLayout legacyLayout={"weather","weather0","weather1","weather_meta"};
    ConfigurationService defaults;defaults.loadConfiguration();Configuration old=defaults.getConfiguration();old.device.name="Old bank";
    std::vector<uint8_t> bytes;TEST_ASSERT_TRUE(encodeConfigurationRecord(old,bytes));
    ConfigurationStorage legacy(legacyLayout);TEST_ASSERT_TRUE(legacy.replaceSnapshot(bytes.data(),bytes.size()));
    ConfigurationService service;service.loadConfiguration();TEST_ASSERT_EQUAL_STRING("Old bank",service.getConfiguration().device.name.c_str());
    old.device.name="New bank";TEST_ASSERT_TRUE(service.restoreConfiguration(old));
    ConfigurationService rebooted;rebooted.loadConfiguration();TEST_ASSERT_EQUAL_STRING("New bank",rebooted.getConfiguration().device.name.c_str());
    TEST_ASSERT_EQUAL_STRING("weather0",legacy.activeNamespace().c_str());
}
void test_dedicated_banks_alternate_and_reset_clears_both_partitions(){
    ConfigurationService service;service.loadConfiguration();service.setWifiPassword("legacy-secret");
    Configuration candidate=service.getConfiguration();candidate.device.name="First";TEST_ASSERT_TRUE(service.restoreConfiguration(candidate));
    candidate.device.name="Second";TEST_ASSERT_TRUE(service.restoreConfiguration(candidate));
    Preferences meta;meta.begin("weather_meta",false,"cfg_nvs");TEST_ASSERT_EQUAL(1,meta.getUInt("active",2));
    TEST_ASSERT_TRUE(service.resetToDefaults());
    for(const char* partition:{"nvs","cfg_nvs"})for(const char* name:{"weather","weather0","weather1"}){
        Preferences p;p.begin(name,false,partition);TEST_ASSERT_FALSE(p.isKey("wifiPassword"));TEST_ASSERT_FALSE(p.isKey("cfg_snapshot"));
    }
    ConfigurationService rebooted;rebooted.loadConfiguration();TEST_ASSERT_EQUAL_STRING("WeatherStation",rebooted.getConfiguration().device.name.c_str());TEST_ASSERT_TRUE(rebooted.getConfiguration().network.wifiPassword.isEmpty());
}
int main(){UNITY_BEGIN();RUN_TEST(test_usb_table_alone_keeps_legacy_settings);RUN_TEST(test_confirmed_restore_migrates_to_dedicated_partition_and_survives_reboot);RUN_TEST(test_failed_migration_preserves_legacy_source_and_next_boot);RUN_TEST(test_missing_partition_keeps_normal_operation_but_does_not_write_a_restore);RUN_TEST(test_existing_default_nvs_bank_is_loaded_before_first_dedicated_restore);RUN_TEST(test_dedicated_banks_alternate_and_reset_clears_both_partitions);return UNITY_END();}
