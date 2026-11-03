// EnvNode: see repository LICENSE.
#include <unity.h>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstring>
#include <limits>
#include <map>
#include "profiles/ProfileRepository.h"
#include "testing/TestWebRoutes.h"
#include "Update.h"
using namespace EnvNode;
using namespace EnvNode::Testing;
UpdateClass Update;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*,size_t n) { return n; }
void setUp() {}
void tearDown() {}
std::vector<uint8_t> file(const char* name) {
    std::ifstream input(std::string("../docs/site/assets/testboard/v1/")+name,std::ios::binary);
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
RemoteCatalog catalog() {
    auto data=file("catalog.cbor");RemoteCatalog c{};
    TEST_ASSERT_TRUE(decodeCatalog(data.data(),data.size(),c));return c;
}
std::vector<uint8_t> encodedCatalog(const RemoteCatalog& c) {
    std::vector<uint8_t> bytes(MaximumCatalogBytes);CompactCborWriter w(bytes.data(),bytes.size());
    w.beginArray(2);w.writeUnsigned(1);w.beginArray(c.count);
    for(size_t i=0;i<c.count;++i) {
        const auto& e=c.entries[i];w.beginArray(7);w.writeText(e.id);w.writeText(e.name);w.writeText(e.identity);
        w.writeText(e.pcb);w.writeText(e.filename);w.writeUnsigned(e.bytes);w.writeText(e.sha256);
    }
    TEST_ASSERT_EQUAL(CompactCborStatus::Success,w.status());bytes.resize(w.size());return bytes;
}
struct Transport : IProfileTransport {
    int calls=0;bool offline=false, badHash=false, badCatalog=false;String expectedHash;
    std::map<std::string,std::vector<uint8_t>> replacements;
    bool fetch(const char* name,size_t maximum,const char* hash,std::unique_ptr<uint8_t[]>& out,size_t& size) override {
        ++calls;out.reset();size=0;expectedHash=hash?hash:"";
        if(offline || (hash && badHash))return false;
        auto data=replacements.count(name)?replacements[name]:file(name);if(data.empty() || data.size()>maximum)return false;
        if(badCatalog && !strcmp(name,"catalog.cbor"))data[1]=2;
        size=data.size();out.reset(new uint8_t[size]);memcpy(out.get(),data.data(),size);return true;
    }
};
void finish(TestRunner& runner,uint32_t& now) {
    for(size_t i=0;i<600 && runner.phase()==Phase::Running;++i) { now+=20;runner.tick(now); }
    TEST_ASSERT_EQUAL(Phase::Review,runner.phase());
}
void test_new_revisions_use_40_inputs_and_forward_reference_paths() {
    for(const char* id:{"mini-0.9-pcb0.9.0","weather-0.3-pcb0.3.0"}) {
        const auto* b=findBoard(id);TEST_ASSERT_NOT_NULL(b);TEST_ASSERT_EQUAL(40,b->inputCount);
        SimulatedFixture fixture;TestRunner runner(fixture);uint32_t now=0;size_t total=0;
        TEST_ASSERT_TRUE(runner.load(*b));
        for(size_t i=0;i<b->count;++i) {
            const auto& c=b->connectors[i];fixture.connect(*b,i,SimulatedFault::None);TEST_ASSERT_TRUE(runner.start(now));finish(runner,now);
            for(size_t p=0;p<c.count;++p) {
                TEST_ASSERT_NOT_EQUAL_UINT64(0,c.contacts[p].expected);
                if(!strcmp(c.contacts[p].net,"GND")) TEST_ASSERT_TRUE((c.contacts[p].expected>>39)&1);
                if(!strcmp(c.contacts[p].net,"+3V3_SYS")) TEST_ASSERT_TRUE((c.contacts[p].expected>>38)&1);
            }
            total+=runner.completed();TEST_ASSERT_EQUAL(Verdict::Passed,runner.verdict(i));runner.next();
        }
        TEST_ASSERT_EQUAL(Verdict::Passed,runner.overall());TEST_ASSERT_EQUAL(b->count==8?2080:3440,total);
    }
    const auto* mini=findBoard("mini-0.9-pcb0.9.0");
    TEST_ASSERT_EQUAL_STRING("J7",mini->connectors[2].id);TEST_ASSERT_EQUAL_STRING("J10",mini->connectors[5].id);
    TEST_ASSERT_TRUE((mini->connectors[7].contacts[2].expected>>38)&1); // Slot B address strap
}
void test_published_profiles_decode_to_exact_builtin_expectations_and_positions() {
    auto c=catalog();TEST_ASSERT_EQUAL(4,c.count);
    for(size_t i=0;i<c.count;++i) {
        const auto& e=c.entries[i];auto data=file(e.filename);auto p=std::unique_ptr<OwnedProfile>(new OwnedProfile());
        TEST_ASSERT_TRUE(decodeProfile(data.data(),data.size(),e,*p));
        const auto* b=findBoard(e.id);TEST_ASSERT_NOT_NULL(b);TEST_ASSERT_EQUAL(b->count,p->board.count);TEST_ASSERT_EQUAL(b->inputCount,p->board.inputCount);
        for(size_t n=0;n<b->inputCount;++n) {
            TEST_ASSERT_EQUAL_STRING(b->socket[n].net,p->inputs[n].net);
            TEST_ASSERT_FLOAT_WITHIN(.0001,b->socket[n].x,p->inputs[n].x);
        }
        for(size_t n=0;n<b->count;++n)for(size_t j=0;j<b->connectors[n].count;++j) {
            TEST_ASSERT_EQUAL_STRING(b->connectors[n].id,p->connectors[n].id);
            TEST_ASSERT_EQUAL_UINT64(b->connectors[n].contacts[j].expected,p->connectors[n].contacts[j].expected);
        }
        const String json=profileJson(*p);
        std::ofstream(std::string("/private/tmp/envnode-decoded-")+e.id+".json") << json.c_str();
        TEST_ASSERT_NOT_NULL(strstr(json.c_str(),"https://docs.envnode.de/assets/testboard/v1/"));
        TEST_ASSERT_NOT_NULL(strstr(json.c_str(),"\"profileOrigin\":\"remote\""));
        if(b->inputCount==40) {
            TEST_ASSERT_NOT_NULL(strstr(json.c_str(),"\"label\":\"MB_3V3_SYS\""));
            TEST_ASSERT_NOT_NULL(strstr(json.c_str(),"\"direction\":\"output-to-input\""));
        }
    }
}
void test_catalog_and_filename_limits_reject_unknown_schema_truncation_and_paths() {
    auto data=file("catalog.cbor");RemoteCatalog c{};
    TEST_ASSERT_FALSE(decodeCatalog(nullptr,0,c));
    data[1]=2;TEST_ASSERT_FALSE(decodeCatalog(data.data(),data.size(),c));data[1]=1;
    TEST_ASSERT_FALSE(decodeCatalog(data.data(),data.size()-1,c));
    data.push_back(0);TEST_ASSERT_FALSE(decodeCatalog(data.data(),data.size(),c));
    TEST_ASSERT_FALSE(safeProfileFilename(nullptr));
    for(const char* path:{"","../test.cbor","https://evil/profile.cbor","a/b.cbor","a\\b.cbor",".secret","abc?x=1"}) TEST_ASSERT_FALSE(safeProfileFilename(path));
    TEST_ASSERT_TRUE(safeProfileFilename("mini-0.9-pcb0.9.0.cbor"));
}
void test_profile_rejects_truncation_metadata_conflicts_and_unknown_schema() {
    auto c=catalog();auto e=c.entries[2];auto data=file(e.filename);auto p=std::unique_ptr<OwnedProfile>(new OwnedProfile());
    TEST_ASSERT_FALSE(decodeProfile(data.data(),data.size()-1,e,*p));
    data[1]=2;TEST_ASSERT_FALSE(decodeProfile(data.data(),data.size(),e,*p));data[1]=1;
    strcpy(e.id,"different");TEST_ASSERT_FALSE(decodeProfile(data.data(),data.size(),e,*p));
    e=c.entries[2];strcpy(e.pcb,"0.3.1");TEST_ASSERT_FALSE(decodeProfile(data.data(),data.size(),e,*p));
    e=c.entries[2];data.push_back(0);e.bytes=data.size();TEST_ASSERT_FALSE(decodeProfile(data.data(),data.size(),e,*p));
}
std::vector<uint8_t> encodedProfile(const BoardDefinition& b, int fault, RemoteProfileEntry& e) {
    std::vector<uint8_t> bytes(MaximumProfileBytes);CompactCborWriter w(bytes.data(),bytes.size());
    e={};strcpy(e.id,b.id);strcpy(e.name,b.name);strcpy(e.identity,b.identityRevision);strcpy(e.pcb,b.pcbRevision);
    w.beginArray(13);w.writeUnsigned(1);w.writeText(e.id);w.writeText(e.name);w.writeText(e.identity);w.writeText(e.pcb);
    w.writeText(std::string(64,'a').c_str());w.writeText("fixture.pcb");w.beginArray(4);
    w.writeDouble(0);w.writeDouble(0);w.writeDouble(fault==1?0:1000);w.writeDouble(1000);
    w.beginArray(b.inputCount);
    for(size_t i=0;i<b.inputCount;++i) {
        w.beginArray(4);w.writeUnsigned(fault==2 && i==1?1:i+1);w.writeText(b.socket[i].net);
        w.writeDouble(fault==3 && !i?std::numeric_limits<double>::quiet_NaN():b.socket[i].x);w.writeDouble(b.socket[i].y);
    }
    w.beginArray(b.count);
    for(size_t i=0;i<b.count;++i) {
        const auto& c=b.connectors[i];w.beginArray(4);w.writeText(fault==4 && i==1?b.connectors[0].id:c.id);w.writeText(c.id);
        w.writeUnsigned(fault==5 && !i?3:static_cast<unsigned>(c.pigtail));w.beginArray(c.count);
        for(size_t j=0;j<c.count;++j) {
            w.beginArray(5);w.writeUnsigned(j+1);w.writeText(fault==8 && !i && !j?"\xff":c.contacts[j].net);w.writeDouble(c.contacts[j].x);w.writeDouble(c.contacts[j].y);
            w.writeUnsigned(fault==6 && !i && !j?1ULL<<40:c.contacts[j].expected);
        }
    }
    w.writeText((std::string(64,'b')+".svg").c_str());w.beginArray(2);
    for(size_t i=0;i<2;++i) {
        w.beginArray(6);w.writeUnsigned(39+i);w.writeText("TP1");w.writeUnsigned(fault==7?1:(i?1:2));
        w.writeText(i?"D1":"D2");w.writeText(fault==9?"+5V":(i?"GND":"+3V3_SYS"));w.writeText(b.socket[38+i].net);
    }
    w.writeText("");TEST_ASSERT_EQUAL(CompactCborStatus::Success,w.status());bytes.resize(w.size());e.bytes=bytes.size();return bytes;
}
void test_profile_semantics_reject_invalid_geometry_indices_masks_and_reference_wiring() {
    const auto& b=*findBoard("mini-0.9-pcb0.9.0");
    for(int fault=0;fault<=9;++fault) {
        RemoteProfileEntry e{};auto data=encodedProfile(b,fault,e);auto p=std::unique_ptr<OwnedProfile>(new OwnedProfile());
        TEST_ASSERT_EQUAL(fault==0,decodeProfile(data.data(),data.size(),e,*p));
    }
}
void test_repository_keeps_embedded_profiles_offline_and_defers_during_measurement() {
    Transport t;t.offline=true;ProfileRepository repo(t);
    repo.service(false,false);repo.service(true,true);TEST_ASSERT_EQUAL(0,t.calls);
    repo.service(true,false);TEST_ASSERT_EQUAL(1,t.calls);TEST_ASSERT_EQUAL(4,repo.count());
    SimulatedFixture f;TestRunner r(f);TEST_ASSERT_TRUE(repo.load("mini-0.9-pcb0.9.0",r));
    TEST_ASSERT_EQUAL_PTR(findBoard("mini-0.9-pcb0.9.0"),r.board());
    f.connect(*r.board(),0,SimulatedFault::None);r.start(0);
    repo.requestRefresh();repo.service(true,true);TEST_ASSERT_FALSE(repo.load("weather-0.3-pcb0.3.0",r));TEST_ASSERT_EQUAL(1,t.calls);
}
void test_remote_load_is_atomic_and_failed_download_falls_back_to_same_revision() {
    Transport t;ProfileRepository repo(t);repo.service(true,false);TEST_ASSERT_EQUAL_STRING("ready",repo.status());TEST_ASSERT_EQUAL(4,repo.count());
    SimulatedFixture f;TestRunner r(f);TEST_ASSERT_TRUE(repo.load("mini-0.9-pcb0.9.0",r));
    TEST_ASSERT_NOT_EQUAL(findBoard("mini-0.9-pcb0.9.0"),r.board());
    const auto* saved=r.board();TEST_ASSERT_NOT_NULL(repo.active(saved->id));TEST_ASSERT_EQUAL(64,t.expectedHash.length());
    TEST_ASSERT_FALSE(repo.load("unknown",r));TEST_ASSERT_EQUAL_PTR(saved,r.board());
    t.badHash=true;TEST_ASSERT_TRUE(repo.load("weather-0.3-pcb0.3.0",r));
    TEST_ASSERT_EQUAL_PTR(findBoard("weather-0.3-pcb0.3.0"),r.board());TEST_ASSERT_NULL(repo.active("mini-0.9-pcb0.9.0"));
    TEST_ASSERT_EQUAL_STRING("profile-fallback",repo.status());
}
void test_failed_catalog_refresh_preserves_active_profile_and_catalog() {
    Transport t;ProfileRepository repo(t);repo.service(true,false);SimulatedFixture f;TestRunner r(f);
    repo.load("mini-0.9-pcb0.9.0",r);const auto* saved=r.board();
    t.badCatalog=true;repo.requestRefresh();repo.service(true,false);
    TEST_ASSERT_EQUAL_PTR(saved,r.board());TEST_ASSERT_EQUAL(4,repo.count());TEST_ASSERT_EQUAL_STRING("unavailable",repo.status());
    RemoteProfileEntry e{};bool remote=false;TEST_ASSERT_TRUE(repo.entryAt(0,e,remote));TEST_ASSERT_TRUE(remote);
}
void test_catalog_revision_conflict_is_rejected_without_touching_active_data() {
    Transport t;ProfileRepository repo(t);repo.service(true,false);SimulatedFixture f;TestRunner r(f);
    repo.load("mini-0.9-pcb0.9.0",r);const auto* saved=r.board();
    auto changed=catalog();changed.entries[0].name[0]='X';t.replacements["catalog.cbor"]=encodedCatalog(changed);
    repo.requestRefresh();repo.service(true,false);
    TEST_ASSERT_EQUAL_STRING("revision-conflict",repo.status());TEST_ASSERT_EQUAL_PTR(saved,r.board());
    RemoteProfileEntry e{};bool remote=false;repo.entryAt(0,e,remote);TEST_ASSERT_EQUAL_STRING("EnvNode Weather",e.name);
}
void test_remote_only_board_can_be_loaded_and_survives_catalog_removal_for_its_session() {
    BoardDefinition custom=*findBoard("mini-0.9-pcb0.9.0");custom.id="sample-1.0-pcb1.0.0";custom.name="Future Sample";
    custom.identityRevision="1.0";custom.pcbRevision="1.0.0";
    RemoteProfileEntry e{};auto bytes=encodedProfile(custom,0,e);strcpy(e.sha256,std::string(64,'c').c_str());
    const auto filename=std::string(e.id)+"."+e.sha256+".cbor";strcpy(e.filename,filename.c_str());
    auto c=catalog();c.entries[c.count++]=e;
    Transport t;t.replacements["catalog.cbor"]=encodedCatalog(c);t.replacements[e.filename]=bytes;
    ProfileRepository repo(t);repo.service(true,false);TEST_ASSERT_EQUAL(5,repo.count());
    SimulatedFixture f;TestRunner r(f);TEST_ASSERT_TRUE(repo.load(e.id,r));TEST_ASSERT_EQUAL_STRING(e.id,r.board()->id);
    const auto* saved=r.board();t.replacements.erase("catalog.cbor");repo.requestRefresh();repo.service(true,false);
    TEST_ASSERT_EQUAL_PTR(saved,r.board());TEST_ASSERT_TRUE(repo.load(e.id,r));
    f.connect(*r.board(),0,SimulatedFault::None);r.start(0);uint32_t now=0;finish(r,now);TEST_ASSERT_EQUAL(Verdict::Passed,r.verdict(0));
    strcpy(e.sha256,std::string(64,'d').c_str());
    strcpy(e.filename,(std::string(e.id)+"."+e.sha256+".cbor").c_str());
    c.entries[4]=e;t.replacements["catalog.cbor"]=encodedCatalog(c);repo.requestRefresh();repo.service(true,false);
    TEST_ASSERT_EQUAL_STRING("revision-conflict",repo.status());TEST_ASSERT_EQUAL_PTR(saved,r.board());
}
struct Logger : ILogger {
    void begin(unsigned long) override {} void println(const char*) override {} void printf(const char*,...) override {}
};
struct Wifi : IWiFiService {
    void begin() override {} void loop() override {} bool connected() const override { return true; }
    bool inSetupAccessPointMode() const override { return false; }
    String ipAddress() const override { return "127.0.0.1"; } String hostname() const override { return "fixture"; }
    int32_t rssi() const override { return -50; }
};
struct Pages : IWebPagePresenter {
    Locale webLocale() const override { return Locale::GermanGermany; }
    void sendPage(const char*,const char*,const String&,int,bool) override {}
};
void test_web_routes_use_the_loaded_remote_model_and_lock_refresh_during_test() {
    Transport t;ProfileRepository repo(t);repo.service(true,false);Logger log;Wifi wifi;Pages pages;WebHost host(log,wifi);
    SimulatedFixture f;TestRunner r(f);TestWebRoutes routes(host,pages,r,f);routes.attachProfiles(repo);routes.registerRoutes(host.server());
    auto& server=host.server();server.request("/test/catalog",HTTP_GET);
    TEST_ASSERT_NOT_NULL(strstr(server.responseBody.c_str(),"\"origin\":\"remote\""));
    server.arguments={{"board","mini-0.9-pcb0.9.0"},{"action","load"},{"generation",String(r.generation())}};
    server.request("/test/action",HTTP_POST);TEST_ASSERT_EQUAL(200,server.responseStatus);
    TEST_ASSERT_NOT_EQUAL(findBoard("mini-0.9-pcb0.9.0"),r.board());
    server.request("/test/definition",HTTP_GET);TEST_ASSERT_EQUAL(200,server.responseStatus);
    TEST_ASSERT_NOT_NULL(strstr(server.responseBody.c_str(),"\"profileOrigin\":\"remote\""));
    TEST_ASSERT_NOT_NULL(strstr(server.responseBody.c_str(),"\"label\":\"MB_GND\""));
    server.arguments={{"action","start"},{"generation",String(r.generation())},{"fault","none"}};
    server.request("/test/action",HTTP_POST);TEST_ASSERT_EQUAL(Phase::Running,r.phase());
    server.arguments={{"action","refresh"},{"generation",String(r.generation())}};
    const int calls=t.calls;server.request("/test/action",HTTP_POST);TEST_ASSERT_EQUAL(409,server.responseStatus);
    TEST_ASSERT_EQUAL(calls,t.calls);
    uint32_t now=0;finish(r,now);server.arguments["generation"]=String(r.generation());
    server.request("/test/action",HTTP_POST);TEST_ASSERT_EQUAL(200,server.responseStatus);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_new_revisions_use_40_inputs_and_forward_reference_paths);
    RUN_TEST(test_published_profiles_decode_to_exact_builtin_expectations_and_positions);
    RUN_TEST(test_catalog_and_filename_limits_reject_unknown_schema_truncation_and_paths);
    RUN_TEST(test_profile_rejects_truncation_metadata_conflicts_and_unknown_schema);
    RUN_TEST(test_profile_semantics_reject_invalid_geometry_indices_masks_and_reference_wiring);
    RUN_TEST(test_repository_keeps_embedded_profiles_offline_and_defers_during_measurement);
    RUN_TEST(test_remote_load_is_atomic_and_failed_download_falls_back_to_same_revision);
    RUN_TEST(test_failed_catalog_refresh_preserves_active_profile_and_catalog);
    RUN_TEST(test_catalog_revision_conflict_is_rejected_without_touching_active_data);
    RUN_TEST(test_remote_only_board_can_be_loaded_and_survives_catalog_removal_for_its_session);
    RUN_TEST(test_web_routes_use_the_loaded_remote_model_and_lock_refresh_during_test);
    return UNITY_END();
}
