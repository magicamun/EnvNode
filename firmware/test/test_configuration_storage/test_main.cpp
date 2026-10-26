// EnvNode: see repository LICENSE.
#include <unity.h>
#include "ConfigurationStorage.h"
#include "BaseSettingsService.h"
#include "EnvNodeConfigurationStorage.h"
#include <string>
#include <cstring>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*,size_t n){return n;}
void pinMode(unsigned char,int){}
void digitalWrite(unsigned char,int){}
constexpr ConfigurationStorageLayout FixtureLayout={"fixture","fixture0","fixture1","fixture_meta"};
constexpr BaseSettingsDefaults FixtureDefaults={"TestFixture","TestFixture","UTC0","one.local","two.local",Locale::GermanGermany};
void setUp(){
    Preferences::failBeginNamespace()=String();Preferences::failClearNamespace()=String();
    Preferences::failStringWrites()=false;Preferences::failByteReads()=false;Preferences::failUIntKey()=String();
    for(const char* name:{"fixture","fixture0","fixture1","fixture_meta","weather","weather0","weather1","weather_meta"}){
        Preferences p;p.begin(name);p.clear();p.end();
    }
}
void tearDown(){Preferences::failBeginNamespace()=String();Preferences::failClearNamespace()=String();Preferences::failStringWrites()=false;Preferences::failByteReads()=false;Preferences::failUIntKey()=String();}
struct Fixture {
    ConfigurationStorage storage{FixtureLayout};DeviceConfiguration device;NetworkConfiguration network;TimeConfiguration time;LocaleConfiguration locale;
    BaseSettingsService settings{storage,device,network,time,locale,FixtureDefaults};
    Fixture(){settings.initializeDefaults();}
    void load(){settings.loadIdentityAndNetworkOverrides();settings.loadTimeAndLocaleOverrides();settings.validateIdentityAndNetwork();settings.validateTime();}
};
uint32_t active(){Preferences p;p.begin("fixture_meta");return p.getUInt("active",2);}
void test_fixed_layout_and_legacy_namespace_are_preserved(){
    TEST_ASSERT_EQUAL_STRING("weather",EnvNodeConfigurationStorage.legacy);
    TEST_ASSERT_EQUAL_STRING("weather0",EnvNodeConfigurationStorage.bank0);
    TEST_ASSERT_EQUAL_STRING("weather1",EnvNodeConfigurationStorage.bank1);
    TEST_ASSERT_EQUAL_STRING("weather_meta",EnvNodeConfigurationStorage.metadata);
    Fixture f;TEST_ASSERT_TRUE(f.storage.begin());TEST_ASSERT_EQUAL_STRING("fixture",f.storage.activeNamespace().c_str());
    TEST_ASSERT_TRUE(f.storage.persistUInt("count",17));TEST_ASSERT_TRUE(f.storage.persistFloat("value",1.25f));
    ConfigurationStorage rebooted(FixtureLayout);TEST_ASSERT_TRUE(rebooted.begin());
    TEST_ASSERT_EQUAL(17,rebooted.preferences().getUInt("count"));TEST_ASSERT_FLOAT_WITHIN(.001f,1.25f,rebooted.preferences().getFloat("value"));
}
void test_base_settings_persist_without_envnode_configuration_model(){
    Fixture f;TEST_ASSERT_EQUAL_STRING("TestFixture",f.device.name.c_str());
    TEST_ASSERT_TRUE(f.settings.setDeviceName("Bench"));TEST_ASSERT_TRUE(f.settings.setWifiSSID("Lab"));
    TEST_ASSERT_TRUE(f.settings.setWifiPassword("secret"));TEST_ASSERT_TRUE(f.settings.setTimezone("CET-1"));
    TEST_ASSERT_TRUE(f.settings.setNtpServer1("new-one"));TEST_ASSERT_TRUE(f.settings.setNtpServer2("new-two"));
    TEST_ASSERT_TRUE(f.settings.setLocale(Locale::EnglishUnitedKingdom));
    Fixture rebooted;rebooted.load();
    TEST_ASSERT_EQUAL_STRING("Bench",rebooted.device.name.c_str());TEST_ASSERT_EQUAL_STRING("Bench",rebooted.network.hostname.c_str());
    TEST_ASSERT_EQUAL_STRING("Lab",rebooted.network.wifiSSID.c_str());TEST_ASSERT_EQUAL_STRING("secret",rebooted.network.wifiPassword.c_str());
    TEST_ASSERT_EQUAL_STRING("CET-1",rebooted.time.timezone.c_str());TEST_ASSERT_EQUAL_STRING("new-one",rebooted.time.ntpServer1.c_str());
    TEST_ASSERT_EQUAL_STRING("new-two",rebooted.time.ntpServer2.c_str());TEST_ASSERT_EQUAL(static_cast<int>(Locale::EnglishUnitedKingdom),static_cast<int>(rebooted.locale.locale));
}
void test_opaque_snapshots_alternate_banks_and_keep_same_bytes(){
    ConfigurationStorage storage(FixtureLayout);const uint8_t first[]={1,2,3,0,4},second[]={9,8};uint8_t read[5]={};
    TEST_ASSERT_TRUE(storage.replaceSnapshot(first,sizeof(first)));TEST_ASSERT_EQUAL(0,active());
    TEST_ASSERT_EQUAL_STRING("fixture0",storage.activeNamespace().c_str());
    TEST_ASSERT_TRUE(storage.persistString("deviceName","Edit"));
    TEST_ASSERT_TRUE(storage.replaceSnapshot(second,sizeof(second)));TEST_ASSERT_EQUAL(1,active());
    TEST_ASSERT_EQUAL_STRING("fixture1",storage.activeNamespace().c_str());TEST_ASSERT_FALSE(storage.preferences().isKey("deviceName"));
    TEST_ASSERT_EQUAL(sizeof(second),storage.preferences().getBytes("cfg_snapshot",read,sizeof(read)));TEST_ASSERT_EQUAL_MEMORY(second,read,sizeof(second));
    ConfigurationStorage rebooted(FixtureLayout);TEST_ASSERT_TRUE(rebooted.begin());TEST_ASSERT_EQUAL_STRING("fixture1",rebooted.activeNamespace().c_str());
}
void test_failed_staging_readback_and_commit_keep_old_active_bank(){
    ConfigurationStorage storage(FixtureLayout);const uint8_t first[]={1,2},second[]={3,4};
    TEST_ASSERT_TRUE(storage.replaceSnapshot(first,sizeof(first)));
    Preferences::failStringWrites()=true;TEST_ASSERT_FALSE(storage.replaceSnapshot(second,sizeof(second)));Preferences::failStringWrites()=false;
    Preferences::failByteReads()=true;TEST_ASSERT_FALSE(storage.replaceSnapshot(second,sizeof(second)));Preferences::failByteReads()=false;
    Preferences::failUIntKey()="active";TEST_ASSERT_FALSE(storage.replaceSnapshot(second,sizeof(second)));Preferences::failUIntKey()=String();
    TEST_ASSERT_EQUAL(0,active());TEST_ASSERT_EQUAL_STRING("fixture0",storage.activeNamespace().c_str());
    uint8_t read[2]={};TEST_ASSERT_EQUAL(2,storage.preferences().getBytes("cfg_snapshot",read,2));TEST_ASSERT_EQUAL_MEMORY(first,read,2);
}
void test_begin_and_inactive_clear_failures_are_reported_and_retryable(){
    ConfigurationStorage storage(FixtureLayout);Preferences::failBeginNamespace()="fixture_meta";
    TEST_ASSERT_FALSE(storage.begin());TEST_ASSERT_FALSE(storage.ready());
    Preferences::failBeginNamespace()=String();TEST_ASSERT_TRUE(storage.begin());
    const uint8_t snapshot[]={1};Preferences::failBeginNamespace()="fixture0";
    TEST_ASSERT_FALSE(storage.replaceSnapshot(snapshot,1));Preferences::failBeginNamespace()=String();
    Preferences::failClearNamespace()="fixture0";TEST_ASSERT_FALSE(storage.replaceSnapshot(snapshot,1));
    Preferences::failClearNamespace()=String();TEST_ASSERT_TRUE(storage.replaceSnapshot(snapshot,1));
}
void test_empty_overrides_keep_restored_snapshot_values_cleared(){
    Fixture f;const uint8_t snapshot[]={1};TEST_ASSERT_TRUE(f.storage.replaceSnapshot(snapshot,1));
    f.network.hostname="Restored-Host";f.network.wifiPassword="restored-secret";f.network.dns2="8.8.4.4";
    TEST_ASSERT_TRUE(f.settings.setWifiPassword(""));TEST_ASSERT_TRUE(f.settings.setDeviceName("Edited"));
    TEST_ASSERT_TRUE(f.storage.persistString("dns2",""));
    f.settings.loadIdentityAndNetworkOverrides();
    TEST_ASSERT_EQUAL_STRING("",f.network.wifiPassword.c_str());TEST_ASSERT_EQUAL_STRING("",f.network.dns2.c_str());
    TEST_ASSERT_EQUAL_STRING("Restored-Host",f.network.hostname.c_str());
}
void test_invalid_values_recover_defaults_and_do_not_write_on_validation_failure(){
    Fixture f;TEST_ASSERT_TRUE(f.storage.begin());
    f.storage.preferences().putString("deviceName",std::string(33,'x').c_str());f.storage.preferences().putString("hostname","bad_host");
    f.storage.preferences().putString("timezone",std::string(129,'x').c_str());f.storage.preferences().putString("ntpServer1",std::string(65,'x').c_str());
    f.storage.preferences().putUInt("locale",99);f.load();
    TEST_ASSERT_EQUAL_STRING("TestFixture",f.device.name.c_str());TEST_ASSERT_EQUAL_STRING("TestFixture",f.network.hostname.c_str());
    TEST_ASSERT_EQUAL_STRING("UTC0",f.time.timezone.c_str());TEST_ASSERT_EQUAL_STRING("one.local",f.time.ntpServer1.c_str());
    TEST_ASSERT_FALSE(f.settings.setDeviceName(""));TEST_ASSERT_FALSE(f.settings.setLocale(static_cast<Locale>(99)));
    TEST_ASSERT_EQUAL_STRING(std::string(33,'x').c_str(),f.storage.preferences().getString("deviceName").c_str());
}
void test_failed_write_keeps_memory_and_preserves_existing_partial_edit_semantics(){
    Fixture f;TEST_ASSERT_TRUE(f.settings.setDeviceName("Before"));
    Preferences::failStringWrites()=true;TEST_ASSERT_FALSE(f.settings.setDeviceName("After"));Preferences::failStringWrites()=false;
    TEST_ASSERT_EQUAL_STRING("Before",f.device.name.c_str());
    NetworkConfiguration request=f.network;request.hostname="New-Host";request.wifiSSID="New-Lab";
    Preferences::failUIntKey()="addressMode";TEST_ASSERT_FALSE(f.settings.setNetworkConfiguration(request,false));Preferences::failUIntKey()=String();
    TEST_ASSERT_EQUAL_STRING("TestFixture",f.network.hostname.c_str());
    TEST_ASSERT_EQUAL_STRING("New-Host",f.storage.preferences().getString("hostname").c_str());
}
void test_network_static_fields_and_password_retention_survive_reboot(){
    Fixture f;TEST_ASSERT_TRUE(f.settings.setWifiPassword("retained-secret"));
    NetworkConfiguration request=f.network;request.hostname="Fixture-Host";request.wifiSSID="Lab";request.wifiPassword="ignored";
    request.addressMode=NetworkAddressMode::Static;request.ipv4Address="192.168.1.20";request.subnetMask="255.255.255.0";
    request.gateway="192.168.1.1";request.dns1="1.1.1.1";request.dns2="";
    TEST_ASSERT_TRUE(f.settings.setNetworkConfiguration(request,false));Fixture rebooted;rebooted.load();
    TEST_ASSERT_EQUAL_STRING("retained-secret",rebooted.network.wifiPassword.c_str());TEST_ASSERT_EQUAL_STRING("192.168.1.20",rebooted.network.ipv4Address.c_str());
    TEST_ASSERT_EQUAL(static_cast<int>(NetworkAddressMode::Static),static_cast<int>(rebooted.network.addressMode));
}
void test_clear_erases_all_own_banks_and_preserves_other_application(){
    Preferences unrelated;unrelated.begin("weather");unrelated.putString("wifiPassword","other-secret");
    ConfigurationStorage storage(FixtureLayout);const uint8_t snapshot[]={1};
    storage.persistString("wifiPassword","old-secret");storage.replaceSnapshot(snapshot,1);storage.replaceSnapshot(snapshot,1);
    TEST_ASSERT_TRUE(storage.clearAll());TEST_ASSERT_EQUAL(1,active());
    for(const char* name:{"fixture","fixture0","fixture1"}){Preferences p;p.begin(name);TEST_ASSERT_FALSE(p.isKey("wifiPassword"));TEST_ASSERT_FALSE(p.isKey("cfg_snapshot"));}
    TEST_ASSERT_EQUAL_STRING("other-secret",unrelated.getString("wifiPassword").c_str());
}
void test_clear_failure_does_not_report_success(){
    ConfigurationStorage storage(FixtureLayout);storage.persistString("deviceName","Before");
    Preferences::failClearNamespace()="fixture0";TEST_ASSERT_FALSE(storage.clearAll());
    TEST_ASSERT_EQUAL_STRING("Before",storage.preferences().getString("deviceName").c_str());
    Preferences::failClearNamespace()=String();TEST_ASSERT_TRUE(storage.clearAll());
}
int main(){UNITY_BEGIN();
    RUN_TEST(test_fixed_layout_and_legacy_namespace_are_preserved);
    RUN_TEST(test_base_settings_persist_without_envnode_configuration_model);
    RUN_TEST(test_opaque_snapshots_alternate_banks_and_keep_same_bytes);
    RUN_TEST(test_failed_staging_readback_and_commit_keep_old_active_bank);
    RUN_TEST(test_begin_and_inactive_clear_failures_are_reported_and_retryable);
    RUN_TEST(test_empty_overrides_keep_restored_snapshot_values_cleared);
    RUN_TEST(test_invalid_values_recover_defaults_and_do_not_write_on_validation_failure);
    RUN_TEST(test_failed_write_keeps_memory_and_preserves_existing_partial_edit_semantics);
    RUN_TEST(test_network_static_fields_and_password_retention_survive_reboot);
    RUN_TEST(test_clear_erases_all_own_banks_and_preserves_other_application);
    RUN_TEST(test_clear_failure_does_not_report_success);
    return UNITY_END();}
