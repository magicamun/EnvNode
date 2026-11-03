// EnvNode: see repository LICENSE.
#include <unity.h>
#include "HardwareProvisioningWebRoutes.h"
#include "HardwareDescriptorCodec.h"
#include "InstanceUuidWebText.h"
#include <cstring>
#include <string>
#include <vector>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t n) { return n; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}
void setUp() {}
void tearDown() {}
struct Logger : ILogger {
    void begin(unsigned long) override {}
    void println(const char*) override {}
    void printf(const char*, ...) override {}
};
struct Pages : IWebPagePresenter {
    Locale locale=Locale::EnglishUnitedKingdom;
    int status=0; String content,title;
    Locale webLocale() const override {return locale;}
    void sendPage(const char* t,const char*,const String& c,int s,bool) override {title=t;content=c;status=s;}
};
struct Storage : IIdentityStorage {
    uint8_t data[32]={};int writes=0;bool failWrite=false;
    bool read(uint16_t address,uint8_t* output,size_t size) override {std::memcpy(output,data+address,size);return true;}
    bool write(uint16_t address,const uint8_t* input,size_t size) override {
        ++writes;if(failWrite)return false;std::memcpy(data+address,input,size);return true;
    }
};
struct Administration : IProvisioningAdministration {
    Storage storage;BoardIdentityStore store{storage};BoardProvisioningService service{store};
    BoardIdentity current={BoardProfileId::EnvNodeMini,{0,8},{"MINI-TEST"}};
    int moduleCalls=0,moduleWrites=0;bool moduleConfirmed=false;
    ModuleSlot slot=ModuleSlot::A;
    ModuleDescriptorProvisioningStatus moduleStatus=ModuleDescriptorProvisioningStatus::Success;
    std::vector<uint8_t> payload;
    BoardIdentity provisioningBoardIdentity() const override {return current;}
    size_t provisioningProfileCount() const override {return 1;}
    bool provisioningProfileAt(size_t index,ProvisioningBoardProfile& profile) const override {
        if(index!=0)return false;profile={BoardProfileId::EnvNodeMini,"Mini <test>",{0,8}};return true;
    }
    bool provisioningProfile(BoardProfileId id,ProvisioningBoardProfile& profile) const override {
        return id==BoardProfileId::EnvNodeMini && provisioningProfileAt(0,profile);
    }
    BoardProvisioningResult provisionBoardIdentity(const BoardIdentity& identity,bool confirmed) override {
        return service.provision(identity,confirmed);
    }
    ModuleDescriptorProvisioningResult provisionModuleDescriptor(ModuleSlot s,const uint8_t* bytes,size_t size,bool confirmed) override {
        ++moduleCalls;moduleConfirmed=confirmed;slot=s;payload.assign(bytes,bytes+size);
        ModuleDescriptorProvisioningResult result;result.slot=s;
        if(!confirmed){result.status=ModuleDescriptorProvisioningStatus::ConfirmationRequired;return result;}
        ++moduleWrites;result.status=moduleStatus;result.bank=HardwareDescriptorBank::B;result.generation=4;return result;
    }
};
struct Fixture {
    WebServer server{80};Pages pages;Logger logger;Administration administration;
    int randomCalls=0;
    HardwareProvisioningWebRoutes routes{server,pages,logger,administration,
        [](){return String("<section>application inventory</section>");},
        [this](uint8_t* output,size_t size){++randomCalls;std::memset(output,0xab,size);}};
    Fixture(){routes.registerRoutes(server);}
    void boardForm(bool confirmed=true){server.arguments={{"boardProfileId","2"},{"serialNumber","MINI-NEW"}};if(confirmed)server.arguments["confirmProvisioning"]="1";}
    void moduleForm(bool confirmed=true){server.arguments={{"descriptorModuleSlot","B"},{"descriptorTemplate","duo-relay-0.3"},{"descriptorSerialNumber","RELAY-1"},{"descriptorProductionBatch","batch-2"},{"descriptorProductionDate","2026-10-25"}};if(confirmed)server.arguments["confirmModuleDescriptorProvisioning"]="1";}
};
void test_hardware_page_preserves_forms_and_application_inventory() {
    Fixture f;TEST_ASSERT_EQUAL(6,f.server.routes.size());
    TEST_ASSERT_TRUE(f.server.request("/hardware",HTTP_GET));
    TEST_ASSERT_EQUAL(200,f.pages.status);
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"application inventory"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"Mini &lt;test&gt; 0.8"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"/hardware/board-identity/write"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"/hardware/module-descriptor/write"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"confirmRemoteProvisioning"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"provision-current-fields"));
    f.pages.locale=Locale::GermanGermany;f.server.request("/hardware",HTTP_GET);
    TEST_ASSERT_EQUAL_STRING(webText(f.pages.locale,TextId::Hardware),f.pages.title.c_str());
}
void test_board_write_verifies_mini_identity_without_changing_active_identity() {
    Fixture f;f.boardForm();f.server.request("/hardware/board-identity/write",HTTP_POST);
    TEST_ASSERT_EQUAL(200,f.pages.status);
    TEST_ASSERT_GREATER_THAN(0,f.administration.storage.writes);
    const auto read=f.administration.store.read();
    TEST_ASSERT_EQUAL_STRING("MINI-NEW",read.identity.serialNumber.value);
    TEST_ASSERT_EQUAL(8,read.identity.revision.minor);
    TEST_ASSERT_EQUAL_STRING("MINI-TEST",f.administration.current.serialNumber.value);
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"action='/restart'"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),webText(f.pages.locale,TextId::RestartLater)));
}
void test_board_write_requires_confirmation_including_legacy_alias() {
    Fixture f;f.boardForm(false);f.server.request("/device/board-identity/write",HTTP_POST);
    TEST_ASSERT_EQUAL(400,f.pages.status);
    TEST_ASSERT_EQUAL(0,f.administration.storage.writes);
    f.server.arguments["confirmProvisioning"]="1";f.server.request("/device/board-identity/write",HTTP_POST);
    TEST_ASSERT_EQUAL(200,f.pages.status);
}
void test_malformed_or_unavailable_profile_and_invalid_serial_never_write() {
    for(const char* profile:{"","2x","65536","9","1"}) {
        Fixture f;f.boardForm();f.server.arguments["boardProfileId"]=profile;
        f.server.request("/hardware/board-identity/write",HTTP_POST);
        TEST_ASSERT_EQUAL(400,f.pages.status);TEST_ASSERT_EQUAL(0,f.administration.storage.writes);
    }
    Fixture f;f.boardForm();f.server.arguments["serialNumber"]="bad<serial>";
    f.server.request("/hardware/board-identity/write",HTTP_POST);
    TEST_ASSERT_EQUAL(400,f.pages.status);TEST_ASSERT_EQUAL(0,f.administration.storage.writes);
}
void test_board_storage_failure_is_a_failed_response() {
    Fixture f;f.boardForm();f.administration.storage.failWrite=true;
    f.server.request("/hardware/board-identity/write",HTTP_POST);
    TEST_ASSERT_EQUAL(400,f.pages.status);
    TEST_ASSERT_EQUAL_STRING(webText(f.pages.locale,TextId::BoardProvisioningFailed),f.pages.title.c_str());
}
void test_module_write_passes_valid_descriptor_manufacturing_and_uuid_v4() {
    Fixture f;f.moduleForm();f.server.request("/hardware/module-descriptor/write",HTTP_POST);
    TEST_ASSERT_EQUAL(200,f.pages.status);TEST_ASSERT_EQUAL(1,f.administration.moduleWrites);
    TEST_ASSERT_EQUAL(static_cast<int>(ModuleSlot::B),static_cast<int>(f.administration.slot));
    HardwareDescriptor descriptor;
    const auto decoded=HardwareDescriptorCodec::decode(f.administration.payload.data(),f.administration.payload.size(),HardwareDescriptorObjectKind::Module,descriptor);
    TEST_ASSERT_EQUAL(static_cast<int>(HardwareDescriptorDecodeStatus::Valid),static_cast<int>(decoded));
    TEST_ASSERT_EQUAL(4,descriptor.instanceId[6]>>4);
    TEST_ASSERT_EQUAL(2,descriptor.instanceId[8]>>6);
    TEST_ASSERT_TRUE(descriptor.serialNumber.equals("RELAY-1"));
    TEST_ASSERT_TRUE(descriptor.productionBatch.equals("batch-2"));
    TEST_ASSERT_TRUE(descriptor.productionDate.equals("2026-10-25"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),descriptorUuid(descriptor.instanceId).c_str()));
}
void test_module_legacy_alias_requires_explicit_confirmation() {
    Fixture f;f.moduleForm(false);f.server.request("/device/module-descriptor/write",HTTP_POST);
    TEST_ASSERT_EQUAL(400,f.pages.status);TEST_ASSERT_FALSE(f.administration.moduleConfirmed);
    TEST_ASSERT_EQUAL(0,f.administration.moduleWrites);
    f.server.arguments["confirmModuleDescriptorProvisioning"]="1";
    f.server.request("/device/module-descriptor/write",HTTP_POST);
    TEST_ASSERT_EQUAL(200,f.pages.status);TEST_ASSERT_EQUAL(1,f.administration.moduleWrites);
}
void test_module_invalid_form_never_calls_provisioning() {
    const char* keys[]={"descriptorModuleSlot","descriptorTemplate","descriptorProductionDate","descriptorSerialNumber","descriptorProductionBatch"};
    const char* values[]={"C","unknown","2026-10-250","12345678901234567890123456789012345678901234567890123456789012345","12345678901234567890123456789012345678901234567890123456789012345"};
    for(size_t i=0;i<5;++i) {
        Fixture f;f.moduleForm();f.server.arguments[keys[i]]=values[i];
        f.server.request("/hardware/module-descriptor/write",HTTP_POST);
        TEST_ASSERT_EQUAL(400,f.pages.status);TEST_ASSERT_EQUAL(0,f.randomCalls);TEST_ASSERT_EQUAL(0,f.administration.moduleCalls);
    }
}
void test_module_policy_and_storage_failures_stay_failed_http_results() {
    const ModuleDescriptorProvisioningStatus statuses[]={ModuleDescriptorProvisioningStatus::IncompatibleDescriptor,ModuleDescriptorProvisioningStatus::StorageUnavailable,ModuleDescriptorProvisioningStatus::WriteFailed,ModuleDescriptorProvisioningStatus::VerificationFailed,ModuleDescriptorProvisioningStatus::RediscoveryFailed};
    for(auto status:statuses) {
        Fixture f;f.moduleForm();f.administration.moduleStatus=status;
        f.server.request("/hardware/module-descriptor/write",HTTP_POST);
        TEST_ASSERT_EQUAL(400,f.pages.status);
        TEST_ASSERT_EQUAL_STRING(webText(f.pages.locale,TextId::ModuleDescriptorProvisioningFailed),f.pages.title.c_str());
    }
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_hardware_page_preserves_forms_and_application_inventory);
    RUN_TEST(test_board_write_verifies_mini_identity_without_changing_active_identity);
    RUN_TEST(test_board_write_requires_confirmation_including_legacy_alias);
    RUN_TEST(test_malformed_or_unavailable_profile_and_invalid_serial_never_write);
    RUN_TEST(test_board_storage_failure_is_a_failed_response);
    RUN_TEST(test_module_write_passes_valid_descriptor_manufacturing_and_uuid_v4);
    RUN_TEST(test_module_legacy_alias_requires_explicit_confirmation);
    RUN_TEST(test_module_invalid_form_never_calls_provisioning);
    RUN_TEST(test_module_policy_and_storage_failures_stay_failed_http_results);
    return UNITY_END();
}
