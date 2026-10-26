#include "ConfigurationTestStorage.h"
// EnvNode: see repository LICENSE.
#include <unity.h>
#include "ConfigurationBackup.h"
#include "ConfigurationService.h"
#include <new>
#include <memory>
#include <string>
using namespace EnvNode;
namespace {
bool rejectIndexAllocation=false,trackAllocations=false;
size_t arrayAllocations=0,objectAllocations=0;
}
// Native fault injection for the explicitly checked allocation paths.
void* operator new[](std::size_t size,const std::nothrow_t&) noexcept {
    if(trackAllocations)++arrayAllocations;
    if(rejectIndexAllocation)return nullptr;
    try{return ::operator new[](size);}catch(...){return nullptr;}
}
void operator delete[](void* value,const std::nothrow_t&) noexcept {::operator delete[](value);}
void* operator new(std::size_t size,const std::nothrow_t&) noexcept {
    if(trackAllocations)++objectAllocations;
    try{return ::operator new(size);}catch(...){return nullptr;}
}
void operator delete(void* value,const std::nothrow_t&) noexcept {::operator delete(value);}
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long){}
void HardwareSerial::println(const char*){}
size_t HardwareSerial::write(const uint8_t*,size_t n){return n;}
void pinMode(unsigned char,int){}
void digitalWrite(unsigned char,int){}
void setUp(){
    clearConfigurationTestStorage();
    rejectIndexAllocation=false;trackAllocations=false;arrayAllocations=objectAllocations=0;
    Preferences::failStringWrites()=false;Preferences::failByteReads()=false;Preferences::failUIntKey()=String();
    for(const char* name:{"weather","weather0","weather1","weather_meta"}){Preferences p;p.begin(name);p.clear();}
}
void tearDown(){rejectIndexAllocation=false;trackAllocations=false;}
void test_owned_preview_decodes_with_one_checked_model_allocation(){
    ConfigurationService service;service.loadConfiguration();service.setDeviceName("Saved name");service.setWifiPassword("secret");
    String backup;TEST_ASSERT_TRUE(encodeConfigurationBackup(service.getConfiguration(),true,"0.7.0",backup));
    std::unique_ptr<Configuration> candidate;bool passwords=false;
    trackAllocations=true;
    const bool ok=decodeConfigurationBackupCandidate(backup,service.getConfiguration(),candidate,passwords);
    trackAllocations=false;
    TEST_ASSERT_TRUE(ok);TEST_ASSERT_EQUAL(1,arrayAllocations);TEST_ASSERT_EQUAL(1,objectAllocations);
    TEST_ASSERT_TRUE(passwords);TEST_ASSERT_NOT_NULL(candidate.get());
    backup=String();
    TEST_ASSERT_EQUAL_STRING("Saved name",candidate->device.name.c_str());
    TEST_ASSERT_EQUAL_STRING("secret",candidate->network.wifiPassword.c_str());
    TEST_ASSERT_TRUE(service.validateRestore(*candidate));
}
void test_index_allocation_failure_returns_without_changing_candidate_or_credentials_flag(){
    ConfigurationService service;service.loadConfiguration();String backup;
    TEST_ASSERT_TRUE(encodeConfigurationBackup(service.getConfiguration(),true,"",backup));
    std::unique_ptr<Configuration> candidate(new Configuration(service.getConfiguration()));
    candidate->device.name="Keep this";Configuration* before=candidate.get();bool passwords=false;
    rejectIndexAllocation=true;trackAllocations=true;
    const bool ok=decodeConfigurationBackupCandidate(backup,service.getConfiguration(),candidate,passwords);
    trackAllocations=false;rejectIndexAllocation=false;
    TEST_ASSERT_FALSE(ok);TEST_ASSERT_EQUAL(1,arrayAllocations);TEST_ASSERT_EQUAL(0,objectAllocations);
    TEST_ASSERT_EQUAL_PTR(before,candidate.get());TEST_ASSERT_EQUAL_STRING("Keep this",candidate->device.name.c_str());
    TEST_ASSERT_FALSE(passwords);
}
void test_legacy_decoder_also_handles_index_allocation_failure(){
    ConfigurationService service;service.loadConfiguration();String backup;
    TEST_ASSERT_TRUE(encodeConfigurationBackup(service.getConfiguration(),true,"",backup));
    Configuration candidate=service.getConfiguration();candidate.device.name="Unchanged";bool passwords=true;
    rejectIndexAllocation=true;
    const bool ok=decodeConfigurationBackup(backup,service.getConfiguration(),candidate,passwords);
    rejectIndexAllocation=false;
    TEST_ASSERT_FALSE(ok);TEST_ASSERT_EQUAL_STRING("Unchanged",candidate.device.name.c_str());TEST_ASSERT_TRUE(passwords);
}
void test_excess_fields_are_rejected_before_index_allocation(){
    ConfigurationService service;service.loadConfiguration();std::string input="{";
    for(size_t i=0;i<1601;++i){if(i)input+=",";input+="\"k"+std::to_string(i)+"\":\"v\"";}input+="}";
    std::unique_ptr<Configuration> candidate;bool passwords=false;
    trackAllocations=true;
    const bool ok=decodeConfigurationBackupCandidate(input.c_str(),service.getConfiguration(),candidate,passwords);
    trackAllocations=false;
    TEST_ASSERT_FALSE(ok);TEST_ASSERT_EQUAL(0,arrayAllocations);TEST_ASSERT_EQUAL(0,objectAllocations);TEST_ASSERT_NULL(candidate.get());
}
void test_owned_preview_preserves_password_omission_and_roundtrip_format(){
    ConfigurationService service;service.loadConfiguration();service.setWifiPassword("kept-secret");
    String backup;TEST_ASSERT_TRUE(encodeConfigurationBackup(service.getConfiguration(),false,"",backup));
    std::unique_ptr<Configuration> owned;bool passwords=true;
    TEST_ASSERT_TRUE(decodeConfigurationBackupCandidate(backup,service.getConfiguration(),owned,passwords));
    TEST_ASSERT_FALSE(passwords);TEST_ASSERT_EQUAL_STRING("kept-secret",owned->network.wifiPassword.c_str());
    Configuration legacy;bool legacyPasswords=true;
    TEST_ASSERT_TRUE(decodeConfigurationBackup(backup,service.getConfiguration(),legacy,legacyPasswords));
    String fromOwned,fromLegacy;
    TEST_ASSERT_TRUE(encodeConfigurationBackup(*owned,true,"",fromOwned));
    TEST_ASSERT_TRUE(encodeConfigurationBackup(legacy,true,"",fromLegacy));
    TEST_ASSERT_EQUAL_STRING(fromLegacy.c_str(),fromOwned.c_str());
}
void test_readback_buffer_allocation_failure_does_not_commit_restore(){
    ConfigurationService service;service.loadConfiguration();service.setDeviceName("Keep active");
    Configuration candidate=service.getConfiguration();candidate.device.name="Not committed";
    rejectIndexAllocation=true;
    const bool saved=service.restoreConfiguration(candidate);
    rejectIndexAllocation=false;
    TEST_ASSERT_FALSE(saved);TEST_ASSERT_EQUAL_STRING("Keep active",service.getConfiguration().device.name.c_str());
    ConfigurationService rebooted;rebooted.loadConfiguration();
    TEST_ASSERT_EQUAL_STRING("Keep active",rebooted.getConfiguration().device.name.c_str());
}
int main(){UNITY_BEGIN();
    RUN_TEST(test_owned_preview_decodes_with_one_checked_model_allocation);
    RUN_TEST(test_index_allocation_failure_returns_without_changing_candidate_or_credentials_flag);
    RUN_TEST(test_legacy_decoder_also_handles_index_allocation_failure);
    RUN_TEST(test_excess_fields_are_rejected_before_index_allocation);
    RUN_TEST(test_owned_preview_preserves_password_omission_and_roundtrip_format);
    RUN_TEST(test_readback_buffer_allocation_failure_does_not_commit_restore);
    return UNITY_END();}
