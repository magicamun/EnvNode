// EnvNode: see repository LICENSE.
#include <unity.h>
#include "SdSystemLog.h"
#include "SdStorageArbiter.h"
#include <cstring>
#include <string>
#include <vector>
using namespace EnvNode;
void pinMode(unsigned char,int) {} void digitalWrite(unsigned char,int) {}
void setUp(){} void tearDown(){}
namespace {
struct Storage : ISdLogStorage {
    SdLogWriteResult result=SdLogWriteResult::Written;
    unsigned calls=0;std::string data;
    SdLogWriteResult append(const char* bytes,size_t size) override {++calls;if(result==SdLogWriteResult::Written) data.append(bytes,size);return result;}
};
void test_queue_overflow_retains_order_and_counts_loss() {
    SdLogQueue queue;LogEntry entry;
    for(unsigned i=0;i<SdLogQueueEntries;++i) {entry.sequence=i;TEST_ASSERT_TRUE(queue.push(entry));}
    TEST_ASSERT_FALSE(queue.push(entry));TEST_ASSERT_FALSE(queue.push(entry));
    TEST_ASSERT_EQUAL_UINT(2,queue.takeLost());TEST_ASSERT_EQUAL_UINT(0,queue.takeLost());
    TEST_ASSERT_EQUAL_UINT(2,queue.dropped());
    for(unsigned i=0;i<SdLogQueueEntries;++i) {TEST_ASSERT_TRUE(queue.pop(entry));TEST_ASSERT_EQUAL_UINT(i,entry.sequence);}
    TEST_ASSERT_FALSE(queue.pop(entry));
    TEST_ASSERT_TRUE(queue.push(entry));queue.clear();TEST_ASSERT_EQUAL_UINT(0,queue.count());
}
void test_jsonl_preserves_time_and_escapes_controls() {
    LogEntry e;e.sequence=17;e.monotonicMs=42;e.level=LogLevel::Error;
    strcpy(e.message,"quotes \" slash \\ tab\t newline\n umlaut ä");
    char output[1800];const size_t length=encodeSdLogEntry(e,output,sizeof(output));
    TEST_ASSERT_GREATER_THAN_UINT(0,length);TEST_ASSERT_EQUAL_CHAR('\n',output[length-1]);
    TEST_ASSERT_NOT_NULL(strstr(output,"\"epoch_s\":null"));
    TEST_ASSERT_NOT_NULL(strstr(output,"\\\" slash \\\\ tab\\u0009 newline\\u000a"));
    TEST_ASSERT_NOT_NULL(strstr(output,"ä"));
    e.wallClockValid=true;e.epochSeconds=1793052000;
    TEST_ASSERT_GREATER_THAN_UINT(0,encodeSdLogEntry(e,output,sizeof(output)));
    TEST_ASSERT_NOT_NULL(strstr(output,"1793052000"));
    TEST_ASSERT_EQUAL_UINT(0,encodeSdLogEntry(e,output,10));
    memset(e.message,1,sizeof(e.message)-1);e.message[255]=0;
    TEST_ASSERT_GREATER_THAN_UINT(0,encodeSdLogEntry(e,output,sizeof(output)));
}
void test_batch_flushes_after_five_seconds_and_handles_clock_wrap() {
    Storage storage;SdLogBatch batch;LogEntry e;strcpy(e.message,"hello");
    TEST_ASSERT_TRUE(batch.add(e,0xffffff00U));
    batch.flush(storage,0xffffff01U);TEST_ASSERT_EQUAL_UINT(0,storage.calls);
    TEST_ASSERT_TRUE(batch.due(5000));batch.flush(storage,5000);
    TEST_ASSERT_EQUAL_UINT(1,storage.calls);TEST_ASSERT_EQUAL_UINT(0,batch.bytes());
    TEST_ASSERT_NOT_NULL(strstr(storage.data.c_str(),"hello"));
}
void test_failure_retains_batch_and_backs_off_busy_does_not_lose_it() {
    Storage storage;SdLogBatch batch;LogEntry e;strcpy(e.message,"retry-me");
    TEST_ASSERT_TRUE(batch.add(e,0));const size_t bytes=batch.bytes();
    storage.result=SdLogWriteResult::Failed;batch.flush(storage,5000);
    TEST_ASSERT_EQUAL_UINT(bytes,batch.bytes());
    batch.flush(storage,34999,true);TEST_ASSERT_EQUAL_UINT(1,storage.calls);
    storage.result=SdLogWriteResult::Busy;batch.flush(storage,35000,true);
    TEST_ASSERT_EQUAL_UINT(2,storage.calls);TEST_ASSERT_EQUAL_UINT(bytes,batch.bytes());
    storage.result=SdLogWriteResult::Written;batch.flush(storage,35001,true);
    TEST_ASSERT_EQUAL_UINT(3,storage.calls);TEST_ASSERT_EQUAL_UINT(0,batch.bytes());
    TEST_ASSERT_NOT_NULL(strstr(storage.data.c_str(),"retry-me"));
}
void test_full_batch_rejects_without_overwriting_and_reports_loss() {
    SdLogBatch batch;LogEntry e;memset(e.message,'x',255);e.message[255]=0;
    unsigned accepted=0;while(batch.add(e,0)) ++accepted;
    TEST_ASSERT_GREATER_THAN_UINT(0,accepted);TEST_ASSERT_LESS_OR_EQUAL_UINT(4096,batch.bytes());
    Storage storage;batch.flush(storage,1,true);TEST_ASSERT_EQUAL_UINT(0,batch.bytes());
    TEST_ASSERT_TRUE(batch.addLoss(27,2));batch.flush(storage,3,true);
    TEST_ASSERT_NOT_NULL(strstr(storage.data.c_str(),"27 entries dropped"));
}
struct Directory : ISdLogDirectory {
    struct File {std::string name;uint64_t bytes;};
    std::vector<File> files;std::vector<std::string> removed;bool readable=true,removable=true;
    bool scan(uint64_t& total,char* oldest,size_t,const char* protectedName) override {
        total=0;oldest[0]=0;if(!readable) return false;
        for(const auto& file:files) if(managedSdLogName(file.name.c_str())) {
            total+=file.bytes;
            if(file.name!=protectedName && (!oldest[0] || file.name<oldest)) strcpy(oldest,file.name.c_str());
        }
        return true;
    }
    bool remove(const char* name) override {
        if(!removable) return false;
        for(auto i=files.begin();i!=files.end();++i) if(i->name==name) {removed.push_back(name);files.erase(i);return true;}
        return false;
    }
};
const char* oldName="boot-0000000001-part-0001.jsonl";
const char* newName="boot-0000000002-part-0001.jsonl";
void test_rotation_only_removes_old_managed_files_and_protects_active_part() {
    TEST_ASSERT_TRUE(managedSdLogName(oldName));TEST_ASSERT_FALSE(managedSdLogName("../test.jsonl"));
    TEST_ASSERT_FALSE(managedSdLogName("boot-0000000001-part-abcd.jsonl"));
    Directory directory;directory.files={{oldName,20*1024*1024},{newName,12*1024*1024},{"keep.txt",999999999}};
    TEST_ASSERT_TRUE(pruneSdSystemLogs(directory,4096,newName));
    TEST_ASSERT_EQUAL_UINT(1,directory.removed.size());TEST_ASSERT_EQUAL_STRING(oldName,directory.removed[0].c_str());
    TEST_ASSERT_EQUAL_UINT(2,directory.files.size());
    Directory full;full.files={{newName,SdLogBudgetBytes}};
    TEST_ASSERT_FALSE(pruneSdSystemLogs(full,1,newName));TEST_ASSERT_TRUE(full.removed.empty());
}
void test_rotation_errors_never_claim_success() {
    Directory directory;directory.readable=false;TEST_ASSERT_FALSE(pruneSdSystemLogs(directory,1,""));
    directory.readable=true;directory.removable=false;directory.files={{oldName,SdLogBudgetBytes}};
    TEST_ASSERT_FALSE(pruneSdSystemLogs(directory,1,""));
    TEST_ASSERT_FALSE(pruneSdSystemLogs(directory,SdLogBudgetBytes+1,""));
}
void test_arbiter_is_exclusive_and_releases_only_owned_lease() {
    {SdStorageLease owner;TEST_ASSERT_TRUE(static_cast<bool>(owner));
    {SdStorageLease blocked;TEST_ASSERT_FALSE(static_cast<bool>(blocked));}
    SdStorageLease stillBlocked;TEST_ASSERT_FALSE(static_cast<bool>(stillBlocked));}
    SdStorageLease next;TEST_ASSERT_TRUE(static_cast<bool>(next));
    next.release();
    SdStorageLease completedWorker;TEST_ASSERT_TRUE(static_cast<bool>(completedWorker));
    next.release(); // Explicit completion and later destruction must not unlock another owner.
    SdStorageLease blockedAgain;TEST_ASSERT_FALSE(static_cast<bool>(blockedAgain));
}
}
int main(){UNITY_BEGIN();
RUN_TEST(test_queue_overflow_retains_order_and_counts_loss);
RUN_TEST(test_jsonl_preserves_time_and_escapes_controls);
RUN_TEST(test_batch_flushes_after_five_seconds_and_handles_clock_wrap);
RUN_TEST(test_failure_retains_batch_and_backs_off_busy_does_not_lose_it);
RUN_TEST(test_full_batch_rejects_without_overwriting_and_reports_loss);
RUN_TEST(test_rotation_only_removes_old_managed_files_and_protects_active_part);
RUN_TEST(test_rotation_errors_never_claim_success);
RUN_TEST(test_arbiter_is_exclusive_and_releases_only_owned_lease);
return UNITY_END();}
