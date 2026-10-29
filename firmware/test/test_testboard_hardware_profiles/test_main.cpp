// EnvNode: see repository LICENSE.
#include <unity.h>
#include <fstream>
#include <iterator>
#include <map>
#include <vector>
#include <cstring>
#include <algorithm>
#include "HardwareProfileRepository.h"
#include "ModuleProfileTemplate.h"
#include "DuoRelayDescriptor.h"
#include "CompactCborCodec.h"
#include "HardwareProvisioningWebRoutes.h"
#include "provisioning/TestBoardProvisioning.h"
#include "provisioning/ProvisioningI2C.h"
#include "IdentityEeprom24LC32.h"
#include "HardwareDescriptorCodec.h"
#include "Update.h"
using namespace EnvNode;
UpdateClass Update;HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*,size_t n){return n;}
void pinMode(unsigned char,int) {}
std::vector<uint8_t> file(const char* name){
    std::ifstream in(std::string("../docs/site/assets/hardware/v1/")+name,std::ios::binary);
    return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
struct Transport : IHardwareProfileTransport {
    bool offline=false,rejectDigest=false;int calls=0,verified=0;
    std::map<std::string,std::vector<uint8_t>> overrides;
    bool fetch(const char* name,size_t max,const char* hash,std::unique_ptr<uint8_t[]>& bytes,size_t& size) override {
        ++calls;bytes.reset();size=0;
        if(offline||(hash&&rejectDigest))return false;
        auto data=overrides.count(name)?overrides[name]:file(name);
        if(data.empty()||data.size()>max)return false;
        if(hash){TEST_ASSERT_EQUAL(64,strlen(hash));TEST_ASSERT_NOT_NULL(strstr(name,hash));++verified;}
        bytes.reset(new uint8_t[data.size()]);memcpy(bytes.get(),data.data(),data.size());size=data.size();return true;
    }
};
struct Logger : ILogger {void begin(unsigned long) override {} void println(const char*) override {} void printf(const char*,...) override {}};
struct Pages : IWebPagePresenter {
    String content;int status=0;
    Locale webLocale() const override {return Locale::GermanGermany;}
    void sendPage(const char*,const char*,const String& value,int code,bool) override {content=value;status=code;}
};
std::vector<uint8_t> catalogBytes(const HardwareProfileCatalog& c){
    std::vector<uint8_t> out(MaximumHardwareCatalogBytes);CompactCborWriter w(out.data(),out.size());
    w.beginArray(2);w.writeUnsigned(1);w.beginArray(c.count);
    for(size_t i=0;i<c.count;++i){const auto& e=c.entries[i];w.beginArray(9);w.writeText(e.id);w.writeText(e.name);w.writeUnsigned(e.kind);
        w.beginArray(2);w.writeUnsigned(e.revision.major);w.writeUnsigned(e.revision.minor);
        w.beginArray(3);w.writeUnsigned(e.minimumFirmware.major);w.writeUnsigned(e.minimumFirmware.minor);w.writeUnsigned(e.minimumFirmware.patch);
        w.writeText(e.image);w.writeText(e.filename);w.writeUnsigned(e.bytes);w.writeText(e.sha256);
    }
    TEST_ASSERT_EQUAL(CompactCborStatus::Success,w.status());out.resize(w.size());return out;
}
std::unique_ptr<HardwareProfileCatalog> catalog(){
    std::unique_ptr<HardwareProfileCatalog> c(new HardwareProfileCatalog);auto bytes=file("provisioning-catalog.cbor");
    TEST_ASSERT_TRUE(decodeHardwareProfileCatalog(bytes.data(),bytes.size(),*c));return c;
}
void setFilename(HardwareProfileEntry& e,const char* id,char hash){
    strcpy(e.id,id);memset(e.sha256,hash,64);e.sha256[64]=0;
    snprintf(e.filename,sizeof(e.filename),"%s.%s.%s.cbor",id,e.sha256,e.kind?"module":"portmap");
}
String hexBytes(const std::vector<uint8_t>& bytes){
    const char* digits="0123456789abcdef";std::string out;out.reserve(bytes.size()*2);
    for(uint8_t byte:bytes){out+=digits[byte>>4];out+=digits[byte&15];}
    return String(out.c_str());
}
std::vector<uint8_t> entryBytes(const HardwareProfileEntry& entry){
    HardwareProfileCatalog c;c.count=1;c.entries[0]=entry;auto bytes=catalogBytes(c);
    TEST_ASSERT_EQUAL_HEX8(0x82,bytes[0]);TEST_ASSERT_EQUAL_HEX8(1,bytes[1]);TEST_ASSERT_EQUAL_HEX8(0x81,bytes[2]);
    return {bytes.begin()+3,bytes.end()};
}
struct Fixture {
    bool external,locked=false;Logger logger;Pages pages;WebServer server{80};Transport transport;HardwareProfileRepository repo{transport};
    ProvisioningI2C bus{16,5};IdentityEeprom24LC32 eeprom{bus,0x50},ea{bus,0x52},eb{bus,0x53};
    BoardIdentityStore enid{eeprom};BoardPortMapStore maps{eeprom};HardwareDescriptorStore a{ea},b{eb};
    TestBoardProvisioning admin{bus,enid,[this](){return !locked;},&a,&b,&maps};
    uint8_t nonce=0;
    HardwareProvisioningWebRoutes routes{server,pages,logger,admin,
        [this](){return String(external?"<section>Tester-Provisioning</section>":"<section>EnvNode-Hardwareverwaltung</section>");},
        [this](uint8_t* bytes,size_t size){memset(bytes,++nonce,size);},options()};
    ProvisioningWebOptions options(){ProvisioningWebOptions o;o.externalTarget=external;o.available=[this](){return !locked;};return o;}
    explicit Fixture(bool tester=true):external(tester){routes.enableRemoteProfiles();routes.registerRoutes(server);}
    void form(const HardwareProfileEntry& e,bool confirmed=true){
        auto bytes=transport.overrides.count(e.filename)?transport.overrides[e.filename]:file(e.filename);
        server.arguments={{"remoteProfileKey",String(e.id)+"|"+e.sha256},{"remoteSerial","PROFILE-TEST"},{"remoteSlot","A"},
            {"remoteProfileEntry",hexBytes(entryBytes(e))},{"remoteProfileData",hexBytes(bytes)}};
        if(confirmed)server.arguments["confirmRemoteProvisioning"]="1";
    }
};
void setUp(){Wire=TwoWire();Wire1=TwoWire();}
void tearDown(){}
void test_generated_catalog_and_all_profiles_decode_and_match_metadata(){
    Transport t;HardwareProfileRepository repo(t);TEST_ASSERT_TRUE(repo.refresh());TEST_ASSERT_EQUAL(6,repo.count());
    for(size_t i=0;i<repo.count();++i){const auto& e=*repo.at(i);const auto* loaded=repo.load(e.id,e.sha256);TEST_ASSERT_NOT_NULL(loaded);TEST_ASSERT_EQUAL(e.bytes,loaded->size);}
    TEST_ASSERT_EQUAL(6,t.verified);
}
void test_catalog_rejects_truncation_duplicate_ids_paths_and_unknown_schema(){
    auto c=catalog();auto bytes=catalogBytes(*c);HardwareProfileCatalog rejected;
    for(size_t n=0;n<bytes.size();++n)TEST_ASSERT_FALSE(decodeHardwareProfileCatalog(bytes.data(),n,rejected));
    auto bad=bytes;bad[1]=2;TEST_ASSERT_FALSE(decodeHardwareProfileCatalog(bad.data(),bad.size(),rejected));
    c->entries[1]=c->entries[0];bad=catalogBytes(*c);TEST_ASSERT_FALSE(decodeHardwareProfileCatalog(bad.data(),bad.size(),rejected));
    c=catalog();strcpy(c->entries[0].image,"assets/../../outside.png");bad=catalogBytes(*c);TEST_ASSERT_FALSE(decodeHardwareProfileCatalog(bad.data(),bad.size(),rejected));
    for(const char* name:{"../catalog.cbor","https://bad.test/a","a%2fb","a/b","a?b",".hidden","a..b"})TEST_ASSERT_FALSE(safeHardwareProfileFilename(name));
}
void test_failed_refresh_and_hash_failure_preserve_catalog_and_loaded_profile(){
    Transport t;HardwareProfileRepository repo(t);TEST_ASSERT_TRUE(repo.refresh());auto e=*repo.at(0);
    const auto* first=repo.load(e.id,e.sha256);TEST_ASSERT_NOT_NULL(first);
    t.offline=true;TEST_ASSERT_FALSE(repo.refresh());TEST_ASSERT_EQUAL(6,repo.count());TEST_ASSERT_EQUAL_PTR(first,repo.load(e.id,e.sha256));
    t.offline=false;t.rejectDigest=true;auto second=*repo.at(1);TEST_ASSERT_NULL(repo.load(second.id,second.sha256));
    TEST_ASSERT_EQUAL_PTR(first,repo.load(e.id,e.sha256));
    TEST_ASSERT_NULL(repo.load(e.id,"stale"));
}
void test_catalog_content_conflict_and_profile_metadata_mismatch_are_rejected(){
    Transport t;HardwareProfileRepository repo(t);TEST_ASSERT_TRUE(repo.refresh());auto c=catalog();
    strcpy(c->entries[0].name,"Conflicting name");t.overrides["provisioning-catalog.cbor"]=catalogBytes(*c);
    TEST_ASSERT_FALSE(repo.refresh());TEST_ASSERT_EQUAL_STRING("identity-conflict",repo.status());TEST_ASSERT_EQUAL(6,repo.count());
    c=catalog();auto bytes=file(c->entries[0].filename);BoardPortMap map;TEST_ASSERT_TRUE(BoardPortMapCodec::decode(bytes.data(),bytes.size(),map));
    strcpy(map.name,"Wrong board");bytes.resize(1024);size_t n=0;TEST_ASSERT_TRUE(BoardPortMapCodec::encode(map,bytes.data(),bytes.size(),n));bytes.resize(n);
    t.overrides[c->entries[0].filename]=bytes;TEST_ASSERT_NULL(repo.load(c->entries[0].id,c->entries[0].sha256));
}
void test_remote_selection_preserves_both_existing_ui_contexts_and_shows_images(){
    for(bool external:{true,false}){Fixture f(external);TEST_ASSERT_TRUE(f.server.request("/hardware",HTTP_GET));TEST_ASSERT_EQUAL(200,f.pages.status);
        TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),external?"Tester-Provisioning":"EnvNode-Hardwareverwaltung"));
        TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),external?"extern angeschlossener Prüfling":"dieser EnvNode"));
        TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"name='boardProfileKey'"));
        TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"remoteProfileKey"));TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"browser-profile-client."));
        TEST_ASSERT_EQUAL(0,f.transport.calls);
        TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"remote-profile-image"));
    }
}
void test_confirmation_stale_selection_and_test_lock_never_write_or_download(){
    Fixture f;TEST_ASSERT_TRUE(f.repo.refresh());auto e=*f.repo.at(0);f.form(e,false);int calls=f.transport.calls;
    TEST_ASSERT_TRUE(f.server.request("/hardware/profile/write",HTTP_POST));TEST_ASSERT_EQUAL(400,f.pages.status);TEST_ASSERT_EQUAL(0,Wire.begins);TEST_ASSERT_EQUAL(calls,f.transport.calls);
    f.form(e);f.server.arguments["remoteProfileKey"]=(std::string(e.id)+"|"+std::string(64,'0')).c_str();
    f.server.request("/hardware/profile/write",HTTP_POST);TEST_ASSERT_EQUAL(400,f.pages.status);TEST_ASSERT_EQUAL(0,Wire.begins);TEST_ASSERT_EQUAL(calls,f.transport.calls);
    f.locked=true;f.form(e);
    TEST_ASSERT_TRUE(f.server.request("/hardware/profile/write",HTTP_POST));TEST_ASSERT_EQUAL(409,f.pages.status);
    TEST_ASSERT_TRUE(f.server.request("/hardware/profile/state",HTTP_GET));TEST_ASSERT_EQUAL(409,f.pages.status);
    TEST_ASSERT_EQUAL(calls,f.transport.calls);TEST_ASSERT_EQUAL(0,Wire.begins);
}
void test_unknown_remote_board_with_higher_minimum_firmware_is_provisioned(){
    Fixture f;auto c=catalog();c->count=1;auto& e=c->entries[0];auto bytes=file(e.filename);BoardPortMap map;
    TEST_ASSERT_TRUE(BoardPortMapCodec::decode(bytes.data(),bytes.size(),map));map.profileId=42;map.revision={0,77};map.minimumFirmware={9,0,0};strcpy(map.name,"Remote-only board");
    e.revision=map.revision;e.minimumFirmware=map.minimumFirmware;strcpy(e.name,map.name);setFilename(e,"remote-only-board",'1');
    bytes.resize(1024);size_t n=0;TEST_ASSERT_TRUE(BoardPortMapCodec::encode(map,bytes.data(),bytes.size(),n));bytes.resize(n);e.bytes=n;
    f.transport.overrides[e.filename]=bytes;f.transport.overrides["provisioning-catalog.cbor"]=catalogBytes(*c);
    TEST_ASSERT_TRUE(f.repo.refresh());f.form(e);f.server.request("/hardware/profile/write",HTTP_POST);TEST_ASSERT_EQUAL(200,f.pages.status);
    TEST_ASSERT_EQUAL(1,Wire.begins);TEST_ASSERT_EQUAL(1,Wire.ends);
    for(const auto& tx:Wire.transmissions)TEST_ASSERT_EQUAL(0x50,tx.address);
    f.bus.begin();BoardPortMap written;TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,f.maps.read(written).status);f.bus.end();
    TEST_ASSERT_EQUAL(42,written.profileId);TEST_ASSERT_EQUAL(9,written.minimumFirmware.major);TEST_ASSERT_EQUAL_STRING("PROFILE-TEST",written.serial.value);
    const auto refreshed=f.admin.refresh();TEST_ASSERT_EQUAL(BoardIdentityStatus::Valid,refreshed.status);TEST_ASSERT_EQUAL(42,encodeBoardProfileId(refreshed.identity.profileId));
}
void test_remote_module_template_generates_unique_instances_and_writes_selected_slot(){
    Fixture f;TEST_ASSERT_TRUE(f.repo.refresh());HardwareProfileEntry module;
    for(size_t i=0;i<f.repo.count();++i)if(f.repo.at(i)->kind)module=*f.repo.at(i);
    uint8_t previous[16]={};
    for(int pass=0;pass<2;++pass){f.form(module);f.server.arguments["remoteSlot"]="B";f.server.arguments["remoteBatch"]="BATCH";
        f.server.request("/hardware/profile/write",HTTP_POST);TEST_ASSERT_EQUAL(200,f.pages.status);
        for(const auto& tx:Wire.transmissions)TEST_ASSERT_EQUAL(0x53,tx.address);
        f.bus.begin();uint8_t payload[2048];auto read=f.b.read(payload,sizeof(payload));f.bus.end();TEST_ASSERT_EQUAL(HardwareDescriptorStoreStatus::Valid,read.status);
        HardwareDescriptor d;TEST_ASSERT_EQUAL(HardwareDescriptorDecodeStatus::Valid,HardwareDescriptorCodec::decode(payload,read.envelope.payloadLength,HardwareDescriptorObjectKind::Module,d));
        TEST_ASSERT_TRUE(d.serialNumber.equals("PROFILE-TEST"));TEST_ASSERT_TRUE(d.productionBatch.equals("BATCH"));
        TEST_ASSERT_EQUAL(0x40,d.instanceId[6]&0xf0);TEST_ASSERT_EQUAL(0x80,d.instanceId[8]&0xc0);
        TEST_ASSERT_NOT_EQUAL(0,memcmp(previous,d.instanceId,16));memcpy(previous,d.instanceId,16);
    }
}
void test_invalid_upload_and_invalid_serial_do_not_connect_eeprom(){
    Fixture f;TEST_ASSERT_TRUE(f.repo.refresh());const auto e=*f.repo.at(0);f.form(e);f.server.arguments["remoteProfileData"]="00";
    f.server.request("/hardware/profile/write",HTTP_POST);TEST_ASSERT_EQUAL(400,f.pages.status);TEST_ASSERT_EQUAL(0,Wire.begins);
    f.form(e);f.server.arguments["remoteSerial"]="invalid<serial>";
    f.server.request("/hardware/profile/write",HTTP_POST);TEST_ASSERT_EQUAL(400,f.pages.status);TEST_ASSERT_EQUAL(0,Wire.begins);
}
void test_remote_only_module_with_unknown_driver_and_future_firmware_is_written(){
    Fixture f;auto c=catalog();HardwareProfileEntry e;
    for(size_t i=0;i<c->count;++i)if(c->entries[i].kind)e=c->entries[i];
    auto bytes=file(e.filename);
    const char type[]="org.envnode.module.duo-relay";
    auto id=std::search(bytes.begin(),bytes.end(),type,type+strlen(type));TEST_ASSERT_TRUE(id!=bytes.end());*id='x';
    const uint8_t version[]={15,0x83,0,5,0},driver[]={18,0x81,0xa2,20,1,21,1};
    auto v=std::search(bytes.begin(),bytes.end(),version,version+sizeof(version));TEST_ASSERT_TRUE(v!=bytes.end());v[2]=9;
    auto d=std::search(bytes.begin(),bytes.end(),driver,driver+sizeof(driver));TEST_ASSERT_TRUE(d!=bytes.end());d[4]=23;
    e.minimumFirmware.major=9;setFilename(e,"remote-only-module",'2');e.bytes=bytes.size();c->count=1;c->entries[0]=e;
    f.transport.overrides[e.filename]=bytes;f.transport.overrides["provisioning-catalog.cbor"]=catalogBytes(*c);
    TEST_ASSERT_TRUE(f.repo.refresh());f.form(e);f.server.request("/hardware/profile/write",HTTP_POST);TEST_ASSERT_EQUAL(200,f.pages.status);
    f.bus.begin();uint8_t written[2048];auto read=f.a.read(written,sizeof(written));f.bus.end();
    TEST_ASSERT_EQUAL(HardwareDescriptorStoreStatus::Valid,read.status);HardwareDescriptor decoded;
    TEST_ASSERT_EQUAL(HardwareDescriptorDecodeStatus::Valid,HardwareDescriptorCodec::decode(written,read.envelope.payloadLength,HardwareDescriptorObjectKind::Module,decoded));
    TEST_ASSERT_TRUE(decoded.typeId.equals("xrg.envnode.module.duo-relay"));TEST_ASSERT_EQUAL(9,decoded.compatibility.minimumFirmwareVersion.major);
    TEST_ASSERT_EQUAL(23,decoded.compatibility.drivers[0].id.code);
}
void test_module_template_preserves_unknown_fields_and_rejects_assigned_templates(){
    auto c=catalog();HardwareProfileEntry e;for(size_t i=0;i<c->count;++i)if(c->entries[i].kind)e=c->entries[i];
    auto bytes=file(e.filename);TEST_ASSERT_EQUAL_HEX8(0xa8,bytes[0]);bytes[0]=0xa9;
    const uint8_t extra[]={0x18,46,0x82,1,2};bytes.insert(bytes.end(),extra,extra+sizeof(extra));
    DuoRelayDescriptorManufacturingData m;m.instanceId[0]=1;m.serialNumber="NEW";
    uint8_t output[2048];size_t size=0;TEST_ASSERT_TRUE(instantiateModuleProfile(bytes.data(),bytes.size(),m,output,sizeof(output),size));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(extra,output+size-sizeof(extra),sizeof(extra));
    size_t invalidSize=0;TEST_ASSERT_FALSE(instantiateModuleProfile(output,size,m,output,sizeof(output),invalidSize));
    TEST_ASSERT_FALSE(instantiateModuleProfile(bytes.data(),bytes.size(),m,output,1,invalidSize));
}
void test_raw_cbor_copy_rejects_trailing_or_noncanonical_values(){
    uint8_t output[32];const uint8_t trailing[]={1,2},noncanonical[]={0x18,1},valid[]={0x82,1,2};
    CompactCborWriter a(output,sizeof(output));TEST_ASSERT_FALSE(a.writeEncodedValue(trailing,sizeof(trailing)));
    CompactCborWriter b(output,sizeof(output));TEST_ASSERT_FALSE(b.writeEncodedValue(noncanonical,sizeof(noncanonical)));
    CompactCborWriter cwriter(output,sizeof(output));TEST_ASSERT_TRUE(cwriter.writeEncodedValue(valid,sizeof(valid)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(valid,output,sizeof(valid));
}

void test_browser_script_is_hash_pinned_and_state_check_does_not_open_i2c(){
    Fixture f;
    TEST_ASSERT_TRUE(f.server.request("/hardware",HTTP_GET));
    TEST_ASSERT_EQUAL(200,f.pages.status);
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"https://docs.envnode.de/assets/hardware/v1/browser-profile-client."));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"integrity='sha256-"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"crossorigin='anonymous'"));
    TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"script async"));
    TEST_ASSERT_TRUE(f.server.request("/hardware/profile/state",HTTP_GET));
    TEST_ASSERT_EQUAL(200,f.server.responseStatus);TEST_ASSERT_EQUAL(0,Wire.begins);TEST_ASSERT_EQUAL(0,f.transport.calls);
    TEST_ASSERT_FALSE(f.server.request("/hardware/profiles/refresh",HTTP_POST));
    // Render an isolated browser smoke-test page; this never connects to a device.
    std::ofstream preview("/private/tmp/envnode-browser-import-preview.html");
    preview<<"<!doctype html><html><head><meta charset='utf-8'><title>EnvNode provisioning test</title></head><body>"<<f.pages.content.c_str()<<"</body></html>";
}
void test_oversized_and_malformed_metadata_uploads_never_touch_eeprom(){
    Fixture f;TEST_ASSERT_TRUE(f.repo.refresh());const auto e=*f.repo.at(0);
    for(const char* field:{"remoteProfileEntry","remoteProfileData"}){
        f.form(e);f.server.arguments[field]=String(std::string(9000,'0').c_str());
        TEST_ASSERT_TRUE(f.server.request("/hardware/profile/write",HTTP_POST));TEST_ASSERT_EQUAL(400,f.pages.status);TEST_ASSERT_EQUAL(0,Wire.begins);
        f.form(e);f.server.arguments[field]="invalid hex";
        f.server.request("/hardware/profile/write",HTTP_POST);TEST_ASSERT_EQUAL(400,f.pages.status);TEST_ASSERT_EQUAL(0,Wire.begins);
    }
}

void test_shared_board_fields_persist_from_builtin_and_remote_forms() {
    for(bool remote:{false,true}) {
        Fixture f(true);
        if(remote){auto c=catalog();f.form(c->entries[1]);}
        else f.server.arguments={{"boardProfileKey","1:0.2"},{"confirmRemoteProvisioning","1"},{"remoteSerial","BOARD-NEW"}};
        f.server.arguments["remoteBatch"]="batch-42";f.server.arguments["remoteDate"]="2026-10-29";
        f.server.request(remote?"/hardware/profile/write":"/hardware/board-identity/write",HTTP_POST);TEST_ASSERT_EQUAL(200,f.pages.status);
        f.admin.refresh();TEST_ASSERT_EQUAL_STRING("batch-42",f.admin.provisioningBoardBatch());TEST_ASSERT_EQUAL_STRING("2026-10-29",f.admin.provisioningBoardDate());
        f.server.request("/hardware",HTTP_GET);TEST_ASSERT_NOT_NULL(strstr(f.pages.content.c_str(),"batch-42"));
    }
}
void test_shared_manufacturing_rejects_oversized_board_fields_without_i2c() {
    Fixture f(true);f.server.arguments={{"boardProfileKey","1:0.2"},{"confirmRemoteProvisioning","1"},{"remoteSerial","BOARD-NEW"},{"remoteBatch",String(std::string(65,'B').c_str())}};
    f.server.request("/hardware/board-identity/write",HTTP_POST);TEST_ASSERT_EQUAL(400,f.pages.status);TEST_ASSERT_EQUAL(0,Wire.begins);
}
int main(){UNITY_BEGIN();
    RUN_TEST(test_shared_board_fields_persist_from_builtin_and_remote_forms);
    RUN_TEST(test_shared_manufacturing_rejects_oversized_board_fields_without_i2c);
    RUN_TEST(test_browser_script_is_hash_pinned_and_state_check_does_not_open_i2c);
    RUN_TEST(test_oversized_and_malformed_metadata_uploads_never_touch_eeprom);
    RUN_TEST(test_remote_only_module_with_unknown_driver_and_future_firmware_is_written);
    RUN_TEST(test_module_template_preserves_unknown_fields_and_rejects_assigned_templates);
    RUN_TEST(test_raw_cbor_copy_rejects_trailing_or_noncanonical_values);
    RUN_TEST(test_generated_catalog_and_all_profiles_decode_and_match_metadata);
    RUN_TEST(test_catalog_rejects_truncation_duplicate_ids_paths_and_unknown_schema);
    RUN_TEST(test_failed_refresh_and_hash_failure_preserve_catalog_and_loaded_profile);
    RUN_TEST(test_catalog_content_conflict_and_profile_metadata_mismatch_are_rejected);
    RUN_TEST(test_remote_selection_preserves_both_existing_ui_contexts_and_shows_images);
    RUN_TEST(test_confirmation_stale_selection_and_test_lock_never_write_or_download);
    RUN_TEST(test_unknown_remote_board_with_higher_minimum_firmware_is_provisioned);
    RUN_TEST(test_remote_module_template_generates_unique_instances_and_writes_selected_slot);
    RUN_TEST(test_invalid_upload_and_invalid_serial_do_not_connect_eeprom);
    return UNITY_END();}
