// EnvNode: see repository LICENSE.
#include <unity.h>
#include "TestMarkerSession.h"
#include "TestMarkerWebView.h"
#include "UdpLogging.h"
#include <cstring>
#include <string>
#include <vector>
#include <cstdlib>
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long){}void HardwareSerial::println(const char*){}
size_t HardwareSerial::write(const uint8_t*,size_t n){return n;}
void pinMode(unsigned char,int){}void digitalWrite(unsigned char,int){}
void setUp(){}void tearDown(){}
struct Clock:ILogTimeProvider {
    uint32_t now=1234;uint32_t monotonicMs()const override{return now;}
    bool wallClockEpochSeconds(int64_t& epoch)const override{epoch=0;return false;}
};
struct Recorder:ITestMarkerRecorder {
    DiagnosticRecordResult result=DiagnosticRecordResult::Accepted;
    unsigned calls=0;DiagnosticEvent last{"invalid"};
    DiagnosticRecordResult recordMarker(const DiagnosticEvent& event)override{++calls;last=event;return result;}
};
struct Buffered:ITestMarkerRecorder {
    BufferedDiagnosticRecorder& recorder;
    explicit Buffered(BufferedDiagnosticRecorder& r):recorder(r){}
    DiagnosticRecordResult recordMarker(const DiagnosticEvent& e)override{return recorder.recordMarker(e);}
};
struct Transport:IUdpLogTransport {
    UdpSendResult result=UdpSendResult::Sent;std::vector<std::string> packets;
    UdpSendResult send(const char* p,size_t n)override{packets.emplace_back(p,n);return result;}
};
UdpLogIdentity identity(){UdpLogIdentity id;strcpy(id.device,"001122aabbcc");strcpy(id.boot,"0123456789abcdef0123456789abcdef");strcpy(id.build,"0.8.7-test");return id;}
const DiagnosticField& field(const DiagnosticEvent& e,const char* key) {
    for(size_t i=0;i<e.count();++i)if(!strcmp(e.field(i).key,key))return e.field(i);
    TEST_FAIL_MESSAGE("Missing marker field");return e.field(0);
}
void contains(const std::string& text,const char* expected){TEST_ASSERT_NOT_NULL(strstr(text.c_str(),expected));}
void test_begin_note_end_share_test_id_label_and_boot_transport_metadata() {
    Clock clock;UdpLogBuffer buffer(clock);Buffered recorder(buffer.diagnostics());TestMarkerSession session;Transport sink;
    TEST_ASSERT_FALSE(buffer.diagnostics().enabled({DiagnosticSourceKind::System,0}));
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Begin,"Head A nass","Controller 0.3",0,recorder));
    TEST_ASSERT_TRUE(session.active());TEST_ASSERT_EQUAL_UINT(1,session.activeId());
    buffer.service(sink,identity());TEST_ASSERT_EQUAL_UINT(1,sink.packets.size());
    contains(sink.packets.back(),"\"event\":\"test_marker\"");contains(sink.packets.back(),"\"phase\":\"begin\"");
    contains(sink.packets.back(),"\"test_id\":1");contains(sink.packets.back(),"\"label\":\"Head A nass\"");
    contains(sink.packets.back(),"\"uptime_ms\":1234");contains(sink.packets.back(),"\"epoch_s\":null");
    contains(sink.packets.back(),identity().boot);contains(sink.packets.back(),"\"source_kind\":\"system\"");
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Note,"ignored","Wasser \"an\"",1,recorder));
    buffer.service(sink,identity());contains(sink.packets.back(),"\"phase\":\"note\"");contains(sink.packets.back(),"Wasser \\\"an\\\"");
    contains(sink.packets.back(),"\"label\":\"Head A nass\"");contains(sink.packets.back(),"\"sequence\":2");
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::End,"ignored","fertig",1,recorder));
    buffer.service(sink,identity());TEST_ASSERT_FALSE(session.active());contains(sink.packets.back(),"\"phase\":\"end\"");
    contains(sink.packets.back(),"\"test_id\":1");contains(sink.packets.back(),"\"sequence\":3");
    TEST_ASSERT_EQUAL_UINT(3,buffer.diagnosticStats().accepted);TEST_ASSERT_EQUAL_UINT(3,buffer.diagnosticStats().sent);
}
void test_stale_forms_and_duplicate_transitions_cannot_mislabel_an_active_test() {
    TestMarkerSession session;Recorder recorder;
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Begin,"A","",0,recorder));
    TEST_ASSERT_EQUAL(TestMarkerResult::Conflict,session.submit(TestMarkerAction::Begin,"B","",0,recorder));
    TEST_ASSERT_EQUAL(TestMarkerResult::Conflict,session.submit(TestMarkerAction::Note,"B","note",0,recorder));
    TEST_ASSERT_EQUAL(TestMarkerResult::Conflict,session.submit(TestMarkerAction::End,"A","",2,recorder));
    TEST_ASSERT_EQUAL_UINT(1,recorder.calls);
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::End,"","",1,recorder));
    TEST_ASSERT_EQUAL(TestMarkerResult::Conflict,session.submit(TestMarkerAction::End,"","",1,recorder));
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Begin,"B","",0,recorder));
    TEST_ASSERT_EQUAL_UINT(2,session.activeId());TEST_ASSERT_EQUAL_STRING("B",session.label());
    TEST_ASSERT_EQUAL(TestMarkerResult::Conflict,session.submit(TestMarkerAction::Note,"","old",1,recorder));
}
void test_standalone_notes_and_new_boot_do_not_invent_test_intervals() {
    TestMarkerSession session;Recorder recorder;
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Note,"als naechstes","Head B",0,recorder));
    TEST_ASSERT_FALSE(session.active());TEST_ASSERT_EQUAL_INT64(0,field(recorder.last,"test_id").integer);
    TEST_ASSERT_EQUAL_STRING("als naechstes",session.label());
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Begin,"B","",0,recorder));
    TEST_ASSERT_EQUAL_UINT(1,session.activeId());
    TestMarkerSession rebooted;TEST_ASSERT_FALSE(rebooted.active());
    TEST_ASSERT_EQUAL(TestMarkerResult::Conflict,rebooted.submit(TestMarkerAction::End,"","",1,recorder));
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,rebooted.submit(TestMarkerAction::Begin,"B","",0,recorder));
    TEST_ASSERT_EQUAL_UINT(1,rebooted.activeId()); // Boot identity disambiguates this ID.
}
void test_text_is_bounded_valid_utf8_and_never_silently_truncated() {
    TestMarkerSession session;Recorder recorder;
    const std::string maximum(32,'x'),tooLong(33,'x');
    TEST_ASSERT_EQUAL(TestMarkerResult::Invalid,session.submit(TestMarkerAction::Begin,"   ","",0,recorder));
    TEST_ASSERT_EQUAL(TestMarkerResult::Invalid,session.submit(TestMarkerAction::Note,"","   ",0,recorder));
    TEST_ASSERT_EQUAL(TestMarkerResult::Invalid,session.submit(TestMarkerAction::Begin,tooLong.c_str(),"",0,recorder));
    TEST_ASSERT_EQUAL(TestMarkerResult::Invalid,session.submit(TestMarkerAction::Begin,"\xc3\x28","",0,recorder));
    TEST_ASSERT_EQUAL_UINT(0,recorder.calls);
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Begin,maximum.c_str(),maximum.c_str(),0,recorder));
    TEST_ASSERT_EQUAL_UINT(32,strlen(field(recorder.last,"label").text));
    TEST_ASSERT_EQUAL(TestMarkerResult::Invalid,session.submit(TestMarkerAction::Note,"",tooLong.c_str(),1,recorder));
    TEST_ASSERT_TRUE(session.active());
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::End,"","ä",1,recorder));
    TEST_ASSERT_EQUAL_STRING("ä",field(recorder.last,"text").text);
}
void test_rejected_admission_does_not_advance_session_or_ids() {
    for(auto result:{DiagnosticRecordResult::Disabled,DiagnosticRecordResult::Unavailable,DiagnosticRecordResult::Full,DiagnosticRecordResult::Busy,DiagnosticRecordResult::Invalid}) {
        TestMarkerSession session;Recorder recorder;recorder.result=result;
        TEST_ASSERT_NOT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Begin,"A","",0,recorder));
        TEST_ASSERT_FALSE(session.active());TEST_ASSERT_EQUAL_STRING("",session.label());
        recorder.result=DiagnosticRecordResult::Accepted;
        TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Begin,"A","",0,recorder));
        TEST_ASSERT_EQUAL_UINT(1,session.activeId());recorder.result=result;
        TEST_ASSERT_NOT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::End,"","",1,recorder));
        TEST_ASSERT_TRUE(session.active());TEST_ASSERT_EQUAL_STRING("A",session.label());
    }
}
void test_marker_survives_checkbox_changes_while_normal_mask_rules_stay_intact() {
    Clock clock;UdpLogBuffer buffer(clock);auto& diag=buffer.diagnostics();Buffered recorder(diag);TestMarkerSession session;
    diag.setMask(DiagnosticSourceKind::Sensor,1);
    TEST_ASSERT_EQUAL(DiagnosticRecordResult::Accepted,diag.record({DiagnosticSourceKind::Sensor,1},DiagnosticEvent("rain_window")));
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Begin,"A","",0,recorder));
    diag.setMask(DiagnosticSourceKind::Sensor,0);diag.setMask(DiagnosticSourceKind::Controller,1);
    DiagnosticRecord record;TEST_ASSERT_TRUE(diag.pop(record));TEST_ASSERT_EQUAL_STRING("test_marker",record.event.name());
    TEST_ASSERT_EQUAL_UINT(1,diag.stats().stale);TEST_ASSERT_FALSE(diag.pop(record));
    TEST_ASSERT_FALSE(diag.enabled({DiagnosticSourceKind::System,0}));
    TEST_ASSERT_EQUAL(DiagnosticRecordResult::Disabled,diag.record({DiagnosticSourceKind::System,0},DiagnosticEvent("runtime_health")));
    TEST_ASSERT_EQUAL(DiagnosticRecordResult::Invalid,diag.recordMarker(DiagnosticEvent("runtime_health")));
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::End,"","",1,recorder));
    TEST_ASSERT_TRUE(diag.pop(record));TEST_ASSERT_EQUAL_STRING("end",field(record.event,"phase").text);
    // A reused queue slot must not inherit the explicit-marker exemption.
    diag.setMask(DiagnosticSourceKind::Sensor,1);
    diag.record({DiagnosticSourceKind::Sensor,1},DiagnosticEvent("rain_window"));diag.setMask(DiagnosticSourceKind::Sensor,0);
    TEST_ASSERT_FALSE(diag.pop(record));TEST_ASSERT_EQUAL_UINT(2,diag.stats().stale);
}
void test_full_buffer_keeps_begin_and_end_state_retryable_and_exposes_sequence_gaps() {
    Clock clock;UdpLogBuffer buffer(clock);auto& diag=buffer.diagnostics();diag.setMask(DiagnosticSourceKind::Sensor,1);
    Buffered recorder(diag);TestMarkerSession session;Transport sink;
    for(unsigned i=0;i<8;++i)TEST_ASSERT_EQUAL(DiagnosticRecordResult::Accepted,diag.record({DiagnosticSourceKind::Sensor,1},DiagnosticEvent("rain_window")));
    TEST_ASSERT_EQUAL(TestMarkerResult::Full,session.submit(TestMarkerAction::Begin,"A","",0,recorder));TEST_ASSERT_FALSE(session.active());
    buffer.service(sink,identity());
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Begin,"A","",0,recorder));
    TEST_ASSERT_EQUAL(TestMarkerResult::Full,session.submit(TestMarkerAction::End,"","",1,recorder));TEST_ASSERT_TRUE(session.active());
    for(unsigned i=0;i<8;++i)buffer.service(sink,identity());
    contains(sink.packets.back(),"\"sequence\":10");contains(sink.packets.back(),"\"test_id\":1");
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::End,"","",1,recorder));buffer.service(sink,identity());
    contains(sink.packets.back(),"\"sequence\":12");TEST_ASSERT_EQUAL_UINT(2,buffer.diagnosticStats().full);
    TEST_ASSERT_FALSE(session.active());
}
void test_unavailable_buffer_and_udp_outage_never_claim_receiver_delivery() {
    Clock clock;UdpLogBuffer buffer(clock);Buffered recorder(buffer.diagnostics());TestMarkerSession session;Transport sink;
    buffer.diagnostics().setAvailable(false);
    TEST_ASSERT_EQUAL(TestMarkerResult::Unavailable,session.submit(TestMarkerAction::Begin,"A","",0,recorder));TEST_ASSERT_FALSE(session.active());
    buffer.diagnostics().setAvailable(true);
    TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Begin,"A","",0,recorder));
    sink.result=UdpSendResult::Offline;buffer.service(sink,identity());
    TEST_ASSERT_TRUE(session.active());TEST_ASSERT_EQUAL_UINT(1,buffer.diagnosticStats().offline);
    TEST_ASSERT_EQUAL_UINT(0,buffer.diagnosticStats().queued);TEST_ASSERT_EQUAL_UINT(0,buffer.diagnosticStats().sent);
}
void test_full_rate_windows_are_preserved_with_regular_test_notes() {
    Clock clock;UdpLogBuffer buffer(clock);buffer.diagnostics().setMask(DiagnosticSourceKind::Sensor,1);
    Buffered recorder(buffer.diagnostics());TestMarkerSession session;
    struct Counter:IUdpLogTransport {
        unsigned rain=0,markers=0;
        UdpSendResult send(const char* p,size_t n)override {
            std::string wire(p,n);
            if(strstr(wire.c_str(),"\"event\":\"rain_window\"")) {
                const char* revision=strstr(wire.c_str(),"\"revision\":");TEST_ASSERT_NOT_NULL(revision);
                TEST_ASSERT_EQUAL_UINT(rain+1,strtoul(revision+11,nullptr,10));++rain;
            } else {contains(wire,"\"test_id\":1");++markers;}
            return UdpSendResult::Sent;
        }
    } counter;
    session.submit(TestMarkerAction::Begin,"Head A","Controller B",0,recorder);buffer.service(counter,identity());
    for(unsigned i=1;i<=14400;++i) {
        clock.now=i*250;DiagnosticEvent event("rain_window");event.integer("revision",i);
        TEST_ASSERT_EQUAL(DiagnosticRecordResult::Accepted,buffer.diagnostics().record({DiagnosticSourceKind::Sensor,1},event));
        if(i%100==0)TEST_ASSERT_EQUAL(TestMarkerResult::Accepted,session.submit(TestMarkerAction::Note,"","Wasser",1,recorder));
        buffer.service(counter,identity());buffer.service(counter,identity());
    }
    session.submit(TestMarkerAction::End,"","fertig",1,recorder);buffer.service(counter,identity());
    TEST_ASSERT_EQUAL_UINT(14400,counter.rain);TEST_ASSERT_EQUAL_UINT(146,counter.markers);
    TEST_ASSERT_EQUAL_UINT(0,buffer.diagnosticStats().full);TEST_ASSERT_EQUAL_UINT(0,buffer.diagnosticStats().encodeError);
}
void test_ui_is_localized_escaped_and_rejects_bad_ids() {
    TestMarkerSession session;Recorder recorder;
    session.submit(TestMarkerAction::Begin,"<script>","",0,recorder);
    const String html=buildTestMarkerHtml(session,Locale::GermanGermany,true,identity().boot);
    contains(html.c_str(),"&lt;script&gt;");contains(html.c_str(),"/network-logging/marker");
    contains(html.c_str(),"name='markerId' value='1'");contains(html.c_str(),"value='begin' disabled");
    contains(html.c_str(),"value='end'>Ende markieren");TEST_ASSERT_TRUE(html.length()<3000);
    const String unavailable=buildTestMarkerHtml(session,Locale::EnglishUnitedKingdom,false,identity().boot);
    contains(unavailable.c_str(),"value='end' disabled");contains(unavailable.c_str(),"Network logging and WiFi");
    contains(html.c_str(),"name='markerBoot'");contains(html.c_str(),identity().boot);
    TEST_ASSERT_TRUE(matchingTestMarkerBoot(identity().boot,identity().boot));
    TEST_ASSERT_FALSE(matchingTestMarkerBoot("ffffffffffffffffffffffffffffffff",identity().boot));
    TEST_ASSERT_FALSE(matchingTestMarkerBoot("",identity().boot));
    uint32_t id=0;TEST_ASSERT_TRUE(parseTestMarkerId("4294967295",id));TEST_ASSERT_EQUAL_UINT(UINT32_MAX,id);
    for(const char* bad:{"","-1","1x","4294967296","10000000000"})TEST_ASSERT_FALSE(parseTestMarkerId(bad,id));
    TEST_ASSERT_EQUAL(303,testMarkerHttpStatus(TestMarkerResult::Accepted));
    TEST_ASSERT_EQUAL(400,testMarkerHttpStatus(TestMarkerResult::Invalid));TEST_ASSERT_EQUAL(409,testMarkerHttpStatus(TestMarkerResult::Conflict));
    TEST_ASSERT_EQUAL(503,testMarkerHttpStatus(TestMarkerResult::Full));
    contains(testMarkerResultText(TestMarkerResult::Accepted,Locale::EnglishUnitedKingdom),"receipt is not confirmed");
    static_assert(sizeof(TestMarkerSession)<=48,"bounded marker session RAM");
}
int main(){UNITY_BEGIN();
    RUN_TEST(test_begin_note_end_share_test_id_label_and_boot_transport_metadata);
    RUN_TEST(test_stale_forms_and_duplicate_transitions_cannot_mislabel_an_active_test);
    RUN_TEST(test_standalone_notes_and_new_boot_do_not_invent_test_intervals);
    RUN_TEST(test_text_is_bounded_valid_utf8_and_never_silently_truncated);
    RUN_TEST(test_rejected_admission_does_not_advance_session_or_ids);
    RUN_TEST(test_marker_survives_checkbox_changes_while_normal_mask_rules_stay_intact);
    RUN_TEST(test_full_buffer_keeps_begin_and_end_state_retryable_and_exposes_sequence_gaps);
    RUN_TEST(test_unavailable_buffer_and_udp_outage_never_claim_receiver_delivery);
    RUN_TEST(test_full_rate_windows_are_preserved_with_regular_test_notes);
    RUN_TEST(test_ui_is_localized_escaped_and_rejects_bad_ids);
    return UNITY_END();}
