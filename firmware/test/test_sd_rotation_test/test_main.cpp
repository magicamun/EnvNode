// EnvNode: see repository LICENSE.
#include <unity.h>
#include "SdRotationTest.h"
#include "SdSystemLog.h"
#include "SdCardWebView.h"
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
using namespace EnvNode;
void pinMode(unsigned char,int) {} void digitalWrite(unsigned char,int) {}
void setUp(){} void tearDown(){}
namespace {
void test_payloads_are_bounded_distinct_and_json_lines() {
    char first[SdRotationTestBatchBytes],last[SdRotationTestBatchBytes];
    TEST_ASSERT_TRUE(fillSdRotationTestBatch(0,first,sizeof(first)));
    TEST_ASSERT_TRUE(fillSdRotationTestBatch(17,last,sizeof(last)));
    TEST_ASSERT_NOT_EQUAL(0,memcmp(first,last,sizeof(first)));
    for(unsigned row=0;row<32;++row) {
        TEST_ASSERT_EQUAL_CHAR('\n',last[row*64+63]);
        const std::string line(last+row*64,63);
        TEST_ASSERT_EQUAL_UINT(0,line.find("{\"rotation_test_batch\":17,\"row\":"+std::to_string(row)+"}"));
        TEST_ASSERT_TRUE(line.find('\0')==std::string::npos);
    }
    TEST_ASSERT_FALSE(fillSdRotationTestBatch(18,last,sizeof(last)));
    TEST_ASSERT_FALSE(fillSdRotationTestBatch(0,last,sizeof(last)-1));
    TEST_ASSERT_FALSE(fillSdRotationTestBatch(0,nullptr,sizeof(last)));
}
void test_all_checks_and_cleanup_are_required_to_pass() {
    SdRotationTestResult result;TEST_ASSERT_FALSE(result.passed());
    result.rotation=result.pruning=result.foreignFile=result.content=true;
    TEST_ASSERT_FALSE(result.passed());
    result.cleanup=true;TEST_ASSERT_TRUE(result.passed());
    result.content=false;TEST_ASSERT_FALSE(result.passed());
    result.content=true;result.foreignFile=false;TEST_ASSERT_FALSE(result.passed());
}
struct Directory : ISdLogDirectory {
    struct File {std::string name;uint64_t bytes;};
    std::vector<File> files;std::vector<std::string> removed;
    bool scan(uint64_t& total,char* oldest,size_t,const char* protectedName) override {
        total=0;oldest[0]=0;
        for(const auto& file:files) if(managedSdLogName(file.name.c_str())) {
            total+=file.bytes;
            if(file.name!=protectedName && (!oldest[0] || file.name<oldest)) strcpy(oldest,file.name.c_str());
        }
        return true;
    }
    bool remove(const char* name) override {
        for(auto i=files.begin();i!=files.end();++i) if(i->name==name) {removed.push_back(name);files.erase(i);return true;}
        return false;
    }
};
void test_small_budget_uses_production_pruning_without_touching_other_files() {
    Directory directory;
    for(unsigned part=1;part<=4;++part) {
        char name[32];snprintf(name,sizeof(name),"boot-0000000001-part-%04u.jsonl",part);
        directory.files.push_back({name,6264});
    }
    directory.files.push_back({"keep.txt",999999999});
    TEST_ASSERT_TRUE(pruneSdSystemLogs(directory,2048,"boot-0000000001-part-0004.jsonl",SdRotationTestBudgetBytes));
    TEST_ASSERT_EQUAL_UINT(1,directory.removed.size());
    TEST_ASSERT_EQUAL_STRING("boot-0000000001-part-0001.jsonl",directory.removed[0].c_str());
    TEST_ASSERT_EQUAL_STRING("keep.txt",directory.files.back().name.c_str());
    TEST_ASSERT_FALSE(pruneSdSystemLogs(directory,1,"",0));
    TEST_ASSERT_FALSE(pruneSdSystemLogs(directory,SdRotationTestBudgetBytes+1,"",SdRotationTestBudgetBytes));
    TEST_ASSERT_EQUAL_UINT(1,directory.removed.size());
}
void test_report_is_localized_and_does_not_hide_cleanup_failure() {
    SdRotationTestResult result;result.rotation=result.pruning=result.foreignFile=result.content=true;
    strcpy(result.directory,"/test<leftover>");
    const String de=renderSdRotationResult(result,Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(de.c_str(),"FAIL"));
    TEST_ASSERT_NOT_NULL(strstr(de.c_str(),"Testreste"));
    TEST_ASSERT_NULL(strstr(de.c_str(),"<leftover>"));
    TEST_ASSERT_NOT_NULL(strstr(de.c_str(),"&lt;leftover&gt;"));
    result.cleanup=true;
    const String en=renderSdRotationResult(result,Locale::EnglishUnitedStates);
    TEST_ASSERT_NULL(strstr(en.c_str(),"FAIL"));
    TEST_ASSERT_NOT_NULL(strstr(en.c_str(),"File contents verified: PASS"));
    TEST_ASSERT_NOT_NULL(strstr(en.c_str(),"method='post' action='/sd-card/rotation-test'"));
}
struct RotationTarget : ISdRotationTestTarget {
    bool mountOk=true,createOk=true,writeOk=true;int cleanupError=0;
    std::string calls;unsigned mounts=0,releases=0;
    bool mount(SdRotationTestResult& result) override {calls+="mount ";++mounts;if(!mountOk) result.error=101;return mountOk;}
    bool createWorkspace(SdRotationTestResult& result) override {calls+="create ";if(!createOk) {result.stage="create";result.error=12;}return createOk;}
    bool exercise(SdRotationTestResult& result) override {calls+="write ";if(!writeOk) {result.stage="write";result.error=13;}return writeOk;}
    void inspect(SdRotationTestResult& result) override {calls+="inspect ";result.rotation=result.pruning=result.foreignFile=result.content=true;}
    int cleanup() override {calls+="cleanup ";return cleanupError;}
    void release() override {calls+="release ";++releases;}
};
void test_transaction_keeps_one_mount_until_verification_and_cleanup() {
    RotationTarget target;SdRotationTestResult result;runSdRotationTest(target,result);
    TEST_ASSERT_TRUE(result.passed());TEST_ASSERT_EQUAL_UINT(1,target.mounts);TEST_ASSERT_EQUAL_UINT(1,target.releases);
    TEST_ASSERT_EQUAL_STRING("mount create write inspect cleanup release ",target.calls.c_str());
}
void test_failed_mount_is_released_without_file_operations() {
    RotationTarget target;target.mountOk=false;SdRotationTestResult result;runSdRotationTest(target,result);
    TEST_ASSERT_FALSE(result.passed());TEST_ASSERT_TRUE(result.cleanup);
    TEST_ASSERT_EQUAL_STRING("mount release ",target.calls.c_str());TEST_ASSERT_EQUAL_INT(101,result.error);
}
void test_cleanup_failure_preserves_original_write_failure() {
    RotationTarget target;target.writeOk=false;target.cleanupError=5;
    SdRotationTestResult result;runSdRotationTest(target,result);
    TEST_ASSERT_FALSE(result.passed());TEST_ASSERT_FALSE(result.cleanup);
    TEST_ASSERT_EQUAL_STRING("write",result.stage);TEST_ASSERT_EQUAL_INT(13,result.error);
    TEST_ASSERT_EQUAL_INT(5,result.cleanupError);
    TEST_ASSERT_EQUAL_STRING("mount create write cleanup release ",target.calls.c_str());
}
void test_creation_failure_still_attempts_cleanup_before_release() {
    RotationTarget target;target.createOk=false;
    SdRotationTestResult result;runSdRotationTest(target,result);
    TEST_ASSERT_FALSE(result.passed());TEST_ASSERT_TRUE(result.cleanup);
    TEST_ASSERT_EQUAL_STRING("mount create cleanup release ",target.calls.c_str());
}
}
int main(){UNITY_BEGIN();
RUN_TEST(test_transaction_keeps_one_mount_until_verification_and_cleanup);
RUN_TEST(test_failed_mount_is_released_without_file_operations);
RUN_TEST(test_cleanup_failure_preserves_original_write_failure);
RUN_TEST(test_creation_failure_still_attempts_cleanup_before_release);
RUN_TEST(test_payloads_are_bounded_distinct_and_json_lines);
RUN_TEST(test_all_checks_and_cleanup_are_required_to_pass);
RUN_TEST(test_small_budget_uses_production_pruning_without_touching_other_files);
RUN_TEST(test_report_is_localized_and_does_not_hide_cleanup_failure);
return UNITY_END();}
