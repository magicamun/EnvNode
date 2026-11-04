// EnvNode: see repository LICENSE.
#include <unity.h>
#include "BufferedDiagnosticRecorder.h"
#include "DiagnosticWebView.h"
#include "SystemDiagnostics.h"
#include "SdSystemLog.h"
#include <thread>
#include <atomic>
#include <cstring>
#include <cmath>
#include <string>
#include <Arduino.h>
using namespace EnvNode;
void pinMode(unsigned char,int){}void digitalWrite(unsigned char,int){}
struct Clock : ILogTimeProvider {uint32_t now=123;bool valid=false;uint32_t monotonicMs() const override{return now;}bool wallClockEpochSeconds(int64_t& epoch) const override{epoch=1793700000;return valid;}};
void setUp(){installDiagnosticRecorder(nullptr);}void tearDown(){installDiagnosticRecorder(nullptr);}
void test_no_storage_is_noop_and_source_policy_is_explicit(){
    DiagnosticEvent event("sample");event.number("raw",12.5);DiagnosticSource sensor(DiagnosticSourceKind::Sensor,1);
    TEST_ASSERT_FALSE(diagnosticRecorder().enabled(sensor));TEST_ASSERT_EQUAL(DiagnosticRecordResult::Unavailable,diagnosticRecorder().record(sensor,event));
    IDiagnosticRecorder& stable=diagnosticRecorder();
    Clock clock;BufferedDiagnosticRecorder recorder(clock);installDiagnosticRecorder(&recorder);
    recorder.setAvailable(true);TEST_ASSERT_EQUAL(DiagnosticRecordResult::Disabled,diagnosticRecorder().record(sensor,event));
    recorder.setMask(DiagnosticSourceKind::Sensor,1);TEST_ASSERT_TRUE(recorder.enabled(sensor));TEST_ASSERT_TRUE(stable.enabled(sensor));
    TEST_ASSERT_EQUAL(DiagnosticRecordResult::Accepted,stable.record(sensor,event));
    TEST_ASSERT_EQUAL(DiagnosticRecordResult::Disabled,recorder.record({DiagnosticSourceKind::Sensor,2},event));
    recorder.setAvailable(false);TEST_ASSERT_EQUAL(DiagnosticRecordResult::Unavailable,recorder.record(sensor,event));
}
void test_queue_is_bounded_and_keeps_time_source_and_order(){
    Clock clock;clock.valid=true;BufferedDiagnosticRecorder r(clock);r.setAvailable(true);r.setMask(DiagnosticSourceKind::Controller,1);
    for(size_t i=0;i<DiagnosticQueueEntries;++i){DiagnosticEvent e("decision");e.integer("index",i);clock.now=100+i;TEST_ASSERT_EQUAL(DiagnosticRecordResult::Accepted,r.record({DiagnosticSourceKind::Controller,1},e));}
    DiagnosticEvent e("overflow");TEST_ASSERT_EQUAL(DiagnosticRecordResult::Full,r.record({DiagnosticSourceKind::Controller,1},e));
    TEST_ASSERT_EQUAL_UINT(1,r.stats().full);TEST_ASSERT_EQUAL_UINT(DiagnosticQueueEntries,r.stats().queued);
    DiagnosticRecord record;
    for(size_t i=0;i<DiagnosticQueueEntries;++i){TEST_ASSERT_TRUE(r.pop(record));TEST_ASSERT_EQUAL_UINT(i+1,record.sequence);TEST_ASSERT_EQUAL_UINT(100+i,record.uptimeMs);TEST_ASSERT_TRUE(record.wallClockValid);TEST_ASSERT_EQUAL_INT64(1793700000,record.epochSeconds);TEST_ASSERT_EQUAL_INT64(i,record.event.field(0).integer);}
    TEST_ASSERT_FALSE(r.pop(record));
}
void test_disabled_sources_and_configuration_generations_do_not_leak_pending_records(){
    Clock c;BufferedDiagnosticRecorder r(c);r.setAvailable(true);r.setMask(DiagnosticSourceKind::Sensor,1);DiagnosticEvent e("sample");
    r.record({DiagnosticSourceKind::Sensor,1},e);r.setMask(DiagnosticSourceKind::Sensor,2);
    r.record({DiagnosticSourceKind::Sensor,2},e);DiagnosticRecord record;TEST_ASSERT_TRUE(r.pop(record));TEST_ASSERT_EQUAL_UINT(2,record.source.id);TEST_ASSERT_EQUAL_UINT(1,r.stats().stale);TEST_ASSERT_FALSE(r.pop(record));
}
void test_invalid_records_are_rejected_without_consuming_queue(){
    Clock c;BufferedDiagnosticRecorder r(c);r.setAvailable(true);r.setMask(DiagnosticSourceKind::Sensor,1);
    DiagnosticEvent bad("bad name");TEST_ASSERT_EQUAL(DiagnosticRecordResult::Invalid,r.record({DiagnosticSourceKind::Sensor,1},bad));
    DiagnosticEvent nonfinite("sample");TEST_ASSERT_FALSE(nonfinite.number("raw",NAN));TEST_ASSERT_FALSE(nonfinite.valid());
    DiagnosticEvent duplicate("sample");duplicate.integer("raw",1);TEST_ASSERT_FALSE(duplicate.integer("raw",2));
    DiagnosticEvent longText("sample");TEST_ASSERT_FALSE(longText.text("text","123456789012345678901234567890123"));
    DiagnosticEvent invalidUtf8("sample");TEST_ASSERT_FALSE(invalidUtf8.text("text","\xc0\xaf"));
    DiagnosticEvent tooMany("sample");for(size_t i=0;i<DiagnosticMaxFields;++i){char name[8];snprintf(name,sizeof(name),"n%u",static_cast<unsigned>(i));TEST_ASSERT_TRUE(tooMany.integer(name,i));}TEST_ASSERT_FALSE(tooMany.integer("extra",7));
    DiagnosticEvent valid("sample");TEST_ASSERT_EQUAL(DiagnosticRecordResult::Invalid,r.record({DiagnosticSourceKind::Sensor,0},valid));TEST_ASSERT_EQUAL(DiagnosticRecordResult::Invalid,r.record({DiagnosticSourceKind::Sensor,33},valid));TEST_ASSERT_EQUAL_UINT(0,r.stats().queued);
}
void test_json_is_typed_escaped_and_bounded(){
    DiagnosticRecord r;r.source={DiagnosticSourceKind::Sensor,3};r.sequence=7;r.uptimeMs=123;
    r.event=DiagnosticEvent("raw_sample");r.event.number("volts",2.5);r.event.integer("counter",INT64_C(9007199254740993));r.event.boolean("wet",true);r.event.text("detail","Regen \"ja\"\n\\");
    char data[2048];const size_t n=encodeDiagnosticRecord(r,data,sizeof(data));TEST_ASSERT_TRUE(n>0);TEST_ASSERT_EQUAL_CHAR('\n',data[n-1]);
    TEST_ASSERT_NOT_NULL(strstr(data,"\"epoch_s\":null"));TEST_ASSERT_NOT_NULL(strstr(data,"\"source_kind\":\"sensor\",\"source_id\":3"));
    TEST_ASSERT_NOT_NULL(strstr(data,"\"volts\":2.5"));TEST_ASSERT_NOT_NULL(strstr(data,"\"counter\":9007199254740993"));TEST_ASSERT_NOT_NULL(strstr(data,"\"wet\":true"));TEST_ASSERT_NOT_NULL(strstr(data,"Regen \\\"ja\\\"\\u000a\\\\"));
    TEST_ASSERT_EQUAL_UINT(0,encodeDiagnosticRecord(r,data,12));TEST_ASSERT_EQUAL_CHAR(0,data[0]);
}
void test_diagnostic_json_uses_shared_batch_retry_and_checkbox_retains_disabled_choice(){
    struct Storage : ISdLogStorage {bool fail=true;std::string data;SdLogWriteResult append(const char* bytes,size_t n) override {if(fail)return SdLogWriteResult::Failed;data.append(bytes,n);return SdLogWriteResult::Written;}} storage;
    DiagnosticRecord record;record.event=DiagnosticEvent("probe");record.event.boolean("ok",true);char line[2048];const size_t n=encodeDiagnosticRecord(record,line,sizeof(line));
    SdLogBatch batch;TEST_ASSERT_TRUE(batch.addEncoded(line,n,100));TEST_ASSERT_FALSE(batch.addEncoded(line,n-1,100));
    TEST_ASSERT_EQUAL(SdLogWriteResult::Failed,batch.flush(storage,5100));storage.fail=false;TEST_ASSERT_EQUAL(SdLogWriteResult::Busy,batch.flush(storage,5101));TEST_ASSERT_EQUAL(SdLogWriteResult::Written,batch.flush(storage,35100));TEST_ASSERT_EQUAL_STRING(line,storage.data.c_str());
    TEST_ASSERT_TRUE(extendedDiagnosticsChoice(true,true,false,false));
    TEST_ASSERT_TRUE(extendedDiagnosticsChoice(true,false,true,false));
    TEST_ASSERT_FALSE(extendedDiagnosticsChoice(false,false,true,true));
    TEST_ASSERT_FALSE(extendedDiagnosticsChoice(true,true,true,false));
    TEST_ASSERT_TRUE(extendedDiagnosticsChoice(false,true,true,true));
    const String disabled=renderExtendedDiagnosticsCheckbox(true,false,Locale::GermanGermany);TEST_ASSERT_NOT_NULL(strstr(disabled.c_str()," checked"));TEST_ASSERT_NOT_NULL(strstr(disabled.c_str()," disabled"));
    const String enabled=renderExtendedDiagnosticsCheckbox(false,true,Locale::EnglishUnitedStates);TEST_ASSERT_NULL(strstr(enabled.c_str()," disabled"));TEST_ASSERT_NOT_NULL(strstr(enabled.c_str(),"Extended diagnostics"));
}
void test_concurrent_producers_and_consumer_preserve_every_accepted_record(){
    Clock clock;BufferedDiagnosticRecorder r(clock);r.setMask(DiagnosticSourceKind::Sensor,3);r.setAvailable(true);
    std::atomic<unsigned> finished{0};std::atomic<unsigned> accepted{0};
    auto producer=[&](unsigned id){DiagnosticEvent e("parallel");e.integer("producer",id);for(unsigned i=0;i<1000;++i)if(r.record({DiagnosticSourceKind::Sensor,static_cast<uint16_t>(id)},e)==DiagnosticRecordResult::Accepted)++accepted;++finished;};
    std::thread a(producer,1),b(producer,2);unsigned consumed=0;DiagnosticRecord record;
    while(finished.load()!=2 || r.stats().queued){if(r.pop(record))++consumed;else std::this_thread::yield();}
    a.join();b.join();TEST_ASSERT_EQUAL_UINT(accepted.load(),consumed);TEST_ASSERT_EQUAL_UINT(2000,r.stats().accepted+r.stats().full+r.stats().busy);TEST_ASSERT_EQUAL_UINT(0,r.stats().queued);
}
void test_maximum_escaped_event_fits_writer_line_bound(){
    char name[DiagnosticNameBytes];memset(name,'a',sizeof(name)-1);name[sizeof(name)-1]=0;DiagnosticRecord r;r.event=DiagnosticEvent(name);
    char text[DiagnosticTextBytes];memset(text,1,sizeof(text)-1);text[sizeof(text)-1]=0;
    for(size_t i=0;i<DiagnosticMaxFields;++i){char key[DiagnosticKeyBytes];memset(key,'a',sizeof(key)-1);key[sizeof(key)-2]='0'+i;key[sizeof(key)-1]=0;TEST_ASSERT_TRUE(r.event.text(key,text));}
    char line[2048];const size_t n=encodeDiagnosticRecord(r,line,sizeof(line));TEST_ASSERT_TRUE(n>0 && n<sizeof(line));TEST_ASSERT_NOT_NULL(strstr(line,"\\u0001"));
}
void test_system_producer_uses_public_recorder_and_respects_availability_and_interval(){
    installDiagnosticRecorder(nullptr);serviceSystemDiagnostics();Clock clock;BufferedDiagnosticRecorder r(clock);r.setMask(DiagnosticSourceKind::System,1);r.setAvailable(true);installDiagnosticRecorder(&r);
    arduinoTestMillis()=1000;serviceSystemDiagnostics();DiagnosticRecord record;TEST_ASSERT_TRUE(r.pop(record));TEST_ASSERT_EQUAL_STRING("runtime_health",record.event.name());TEST_ASSERT_EQUAL_STRING("free_heap_8bit",record.event.field(0).key);TEST_ASSERT_EQUAL_INT64(200000,record.event.field(0).integer);TEST_ASSERT_EQUAL_STRING("largest_heap_8bit",record.event.field(1).key);
    serviceSystemDiagnostics();TEST_ASSERT_FALSE(r.pop(record));arduinoTestMillis()+=60000;serviceSystemDiagnostics();TEST_ASSERT_TRUE(r.pop(record));
    r.setAvailable(false);serviceSystemDiagnostics();r.setAvailable(true);serviceSystemDiagnostics();TEST_ASSERT_TRUE(r.pop(record));
    TEST_ASSERT_EQUAL(DiagnosticRecordResult::Accepted,recordDiagnosticTest());TEST_ASSERT_TRUE(r.pop(record));TEST_ASSERT_EQUAL_STRING("recorder_test",record.event.name());
    installDiagnosticRecorder(nullptr);serviceSystemDiagnostics();
}
void test_stopping_admission_preserves_accepted_records_for_eject_drain() {
    Clock clock;BufferedDiagnosticRecorder recorder(clock);
    recorder.setAvailable(true);recorder.setMask(DiagnosticSourceKind::Sensor,1);
    DiagnosticSource source(DiagnosticSourceKind::Sensor,1);DiagnosticEvent event("rain_window");
    event.integer("revision",1);
    TEST_ASSERT_EQUAL(DiagnosticRecordResult::Accepted,recorder.record(source,event));
    recorder.setAvailable(false);TEST_ASSERT_FALSE(recorder.enabled(source));
    TEST_ASSERT_EQUAL(DiagnosticRecordResult::Unavailable,recorder.record(source,event));
    TEST_ASSERT_FALSE(recorder.empty());DiagnosticRecord record;
    TEST_ASSERT_TRUE(recorder.pop(record));TEST_ASSERT_EQUAL_UINT(1,record.sequence);
    TEST_ASSERT_TRUE(recorder.empty());
    recorder.setAvailable(true);recorder.record(source,event);
    TEST_ASSERT_TRUE(recorder.discardPending());TEST_ASSERT_TRUE(recorder.empty());
    TEST_ASSERT_EQUAL_UINT(1,recorder.stats().stale);
}
int main(){UNITY_BEGIN();RUN_TEST(test_stopping_admission_preserves_accepted_records_for_eject_drain);RUN_TEST(test_system_producer_uses_public_recorder_and_respects_availability_and_interval);RUN_TEST(test_maximum_escaped_event_fits_writer_line_bound);RUN_TEST(test_diagnostic_json_uses_shared_batch_retry_and_checkbox_retains_disabled_choice);RUN_TEST(test_concurrent_producers_and_consumer_preserve_every_accepted_record);RUN_TEST(test_no_storage_is_noop_and_source_policy_is_explicit);RUN_TEST(test_queue_is_bounded_and_keeps_time_source_and_order);RUN_TEST(test_disabled_sources_and_configuration_generations_do_not_leak_pending_records);RUN_TEST(test_invalid_records_are_rejected_without_consuming_queue);RUN_TEST(test_json_is_typed_escaped_and_bounded);return UNITY_END();}
