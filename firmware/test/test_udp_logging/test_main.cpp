// EnvNode-owned: see repository LICENSE.
#include <unity.h>
#include "UdpLogging.h"
#include <cstring>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
using namespace EnvNode;
void pinMode(unsigned char,int){}void digitalWrite(unsigned char,int){}
void setUp(){}void tearDown(){}
struct Clock:ILogTimeProvider {
    uint32_t now=0;uint32_t monotonicMs()const override{return now;}
    bool wallClockEpochSeconds(int64_t& epoch)const override{epoch=0;return false;}
};
UdpLogIdentity identity(){UdpLogIdentity id;strcpy(id.device,"001122aabbcc");strcpy(id.boot,"0123456789abcdef0123456789abcdef");strcpy(id.build,"0.8.7+123.gabcdef0.dirty");return id;}
struct Transport:IUdpLogTransport {
    UdpSendResult result=UdpSendResult::Sent;std::vector<std::string> packets;
    UdpSendResult send(const char* bytes,size_t n)override{packets.emplace_back(bytes,n);return result;}
};
void test_format_preserves_typed_fields_and_identity(){
    auto id=identity();DiagnosticRecord r;r.sequence=42;r.uptimeMs=250;r.generation=2;r.source={DiagnosticSourceKind::Sensor,1};
    r.event=DiagnosticEvent("rain_window");r.event.integer("revision",13);r.event.integer("start_us",0);r.event.integer("end_us",250000);r.event.integer("pulses",123);r.event.number("frequency_hz",492);r.event.text("fault","none");
    char out[UdpLogMaxBytes];size_t n=encodeSyslogDiagnostic(id,r,out,sizeof(out));TEST_ASSERT_TRUE(n>0&&n<1200);
    TEST_ASSERT_NOT_NULL(strstr(out,"<134>1 - 001122aabbcc envnode-diagnostic "));
    TEST_ASSERT_NOT_NULL(strstr(out,"\"revision\":13"));TEST_ASSERT_NOT_NULL(strstr(out,"\"epoch_s\":null"));
    TEST_ASSERT_NOT_NULL(strstr(out,"\"selection_generation\":2"));TEST_ASSERT_NOT_NULL(strstr(out,id.boot));
    TEST_ASSERT_NOT_NULL(strstr(out,"\"sequence\":42"));TEST_ASSERT_EQUAL_CHAR('}',out[n-1]);
    LogEntry entry;entry.sequence=9;entry.level=LogLevel::Warn;strcpy(entry.message,"a\"b\n\\");entry.wallClockValid=true;entry.epochSeconds=1793700000;
    n=encodeSyslogSystem(id,entry,out,sizeof(out));TEST_ASSERT_TRUE(n>0);TEST_ASSERT_NOT_NULL(strstr(out,"<132>1"));TEST_ASSERT_NOT_NULL(strstr(out,"envnode-system"));
    TEST_ASSERT_NOT_NULL(strstr(out,"a\\\"b\\u000a\\\\"));TEST_ASSERT_NOT_NULL(strstr(out,"1793700000"));
    TEST_ASSERT_EQUAL_UINT(0,encodeSyslogSystem(id,entry,out,10));TEST_ASSERT_EQUAL_CHAR(0,out[0]);
    strcpy(id.device,"bad host");TEST_ASSERT_EQUAL_UINT(0,encodeSyslogSystem(id,entry,out,sizeof(out)));
}
void test_maximum_existing_events_are_not_truncated(){
    auto id=identity();memset(id.build,'a',sizeof(id.build)-1);id.build[sizeof(id.build)-1]=0;
    DiagnosticRecord r;r.source={DiagnosticSourceKind::Controller,32};r.sequence=UINT32_MAX;r.uptimeMs=UINT32_MAX;r.generation=UINT32_MAX;r.wallClockValid=true;r.epochSeconds=INT64_MAX;
    char name[DiagnosticNameBytes];memset(name,'a',sizeof(name)-1);name[sizeof(name)-1]=0;r.event=DiagnosticEvent(name);
    char text[DiagnosticTextBytes];memset(text,1,sizeof(text)-1);text[sizeof(text)-1]=0;
    for(size_t i=0;i<DiagnosticMaxFields;++i){char key[DiagnosticKeyBytes];memset(key,'a',sizeof(key)-1);key[sizeof(key)-2]='0'+i;key[sizeof(key)-1]=0;TEST_ASSERT_TRUE(r.event.text(key,text));}
    char out[UdpLogMaxBytes];size_t n=encodeSyslogDiagnostic(id,r,out,sizeof(out));TEST_ASSERT_TRUE(n>1200&&n<sizeof(out));TEST_ASSERT_EQUAL_CHAR('}',out[n-1]);
    LogEntry entry;memset(entry.message,1,sizeof(entry.message)-1);entry.message[sizeof(entry.message)-1]=0;
    n=encodeSyslogSystem(id,entry,out,sizeof(out));TEST_ASSERT_TRUE(n>0&&n<sizeof(out));
}
void test_full_queues_preserve_sequence_gaps_and_fairness(){
    Clock c;UdpLogBuffer b(c);TEST_ASSERT_TRUE(b.available());b.setSystemEnabled(true);b.diagnostics().setMask(DiagnosticSourceKind::Sensor,1);
    DiagnosticEvent e("rain_window");LogEntry log;
    for(unsigned i=0;i<8;++i){log.sequence=i;TEST_ASSERT_TRUE(b.recordSystem(log));TEST_ASSERT_EQUAL(DiagnosticRecordResult::Accepted,b.diagnostics().record({DiagnosticSourceKind::Sensor,1},e));}
    TEST_ASSERT_FALSE(b.recordSystem(log));TEST_ASSERT_EQUAL(DiagnosticRecordResult::Full,b.diagnostics().record({DiagnosticSourceKind::Sensor,1},e));
    Transport t;b.service(t,identity());TEST_ASSERT_EQUAL_UINT(2,t.packets.size());TEST_ASSERT_EQUAL_UINT(7,b.systemStats().queued);TEST_ASSERT_EQUAL_UINT(7,b.diagnosticStats().queued);
    TEST_ASSERT_EQUAL(DiagnosticRecordResult::Accepted,b.diagnostics().record({DiagnosticSourceKind::Sensor,1},e));
    for(unsigned i=0;i<8;++i)b.service(t,identity());
    TEST_ASSERT_EQUAL_UINT(0,b.diagnosticStats().queued);TEST_ASSERT_EQUAL_UINT(1,b.diagnosticStats().full);
    TEST_ASSERT_NOT_NULL(strstr(t.packets.back().c_str(),"\"sequence\":10"));TEST_ASSERT_EQUAL_UINT(8,b.diagnosticStats().highWater);
}
void test_offline_backpressure_and_errors_discard_without_retry(){
    Clock c;UdpLogBuffer b(c);b.setSystemEnabled(true);b.diagnostics().setMask(DiagnosticSourceKind::Sensor,1);Transport t;DiagnosticEvent e("rain_window");
    for(auto result:{UdpSendResult::Offline,UdpSendResult::WouldBlock,UdpSendResult::Failed}) {
        t.result=result;LogEntry log;b.recordSystem(log);b.diagnostics().record({DiagnosticSourceKind::Sensor,1},e);
        b.service(t,identity());TEST_ASSERT_EQUAL_UINT(0,b.systemStats().queued);TEST_ASSERT_EQUAL_UINT(0,b.diagnosticStats().queued);
    }
    b.service(t,identity());TEST_ASSERT_EQUAL_UINT(6,t.packets.size());
    auto s=b.diagnosticStats();TEST_ASSERT_EQUAL_UINT(1,s.offline);TEST_ASSERT_EQUAL_UINT(1,s.wouldBlock);TEST_ASSERT_EQUAL_UINT(1,s.failed);
    t.result=UdpSendResult::Sent;b.diagnostics().record({DiagnosticSourceKind::Sensor,1},e);b.service(t,identity());TEST_ASSERT_EQUAL_UINT(1,b.diagnosticStats().sent);
}
void test_one_hour_full_rate_and_controller_bursts(){
    Clock c;UdpLogBuffer b(c);b.setSystemEnabled(true);b.diagnostics().setMask(DiagnosticSourceKind::Sensor,1);b.diagnostics().setMask(DiagnosticSourceKind::Controller,1);
    struct Sink:IUdpLogTransport {unsigned count=0;UdpSendResult send(const char*,size_t)override{++count;return UdpSendResult::Sent;}} sink;
    for(unsigned i=0;i<14400;++i) {
        c.now=i*250;DiagnosticEvent rain("rain_window");rain.integer("revision",i+1);
        TEST_ASSERT_EQUAL(DiagnosticRecordResult::Accepted,b.diagnostics().record({DiagnosticSourceKind::Sensor,1},rain));
        if(i%4==0){for(unsigned j=0;j<4;++j){DiagnosticEvent e("controller_decision");e.integer("revision",i+1);TEST_ASSERT_EQUAL(DiagnosticRecordResult::Accepted,b.diagnostics().record({DiagnosticSourceKind::Controller,1},e));}LogEntry log;log.sequence=i;b.recordSystem(log);}
        for(unsigned j=0;j<5;++j)b.service(sink,identity());
    }
    TEST_ASSERT_EQUAL_UINT(32400,sink.count);TEST_ASSERT_EQUAL_UINT(0,b.diagnosticStats().full);TEST_ASSERT_EQUAL_UINT(0,b.diagnosticStats().encodeError);
}
void test_real_udp_loopback_preserves_all_14400_rain_windows() {
    int receiver=socket(AF_INET,SOCK_DGRAM,0);TEST_ASSERT_TRUE(receiver>=0);
    sockaddr_in destination{};destination.sin_family=AF_INET;destination.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    TEST_ASSERT_EQUAL_INT(0,bind(receiver,reinterpret_cast<sockaddr*>(&destination),sizeof(destination)));
    socklen_t size=sizeof(destination);TEST_ASSERT_EQUAL_INT(0,getsockname(receiver,reinterpret_cast<sockaddr*>(&destination),&size));
    TEST_ASSERT_EQUAL_INT(0,fcntl(receiver,F_SETFL,O_NONBLOCK));
    struct Socket:IUdpLogTransport {
        int fd; sockaddr_in address;
        UdpSendResult send(const char* bytes,size_t n)override{return sendto(fd,bytes,n,MSG_DONTWAIT,reinterpret_cast<sockaddr*>(&address),sizeof(address))==static_cast<int>(n)?UdpSendResult::Sent:UdpSendResult::Failed;}
    } transport;
    transport.fd=socket(AF_INET,SOCK_DGRAM,0);transport.address=destination;TEST_ASSERT_TRUE(transport.fd>=0);
    Clock clock;UdpLogBuffer buffer(clock);buffer.diagnostics().setMask(DiagnosticSourceKind::Sensor,1);
    for(unsigned i=1;i<=14400;++i) {
        clock.now=i*250;DiagnosticEvent event("rain_window");event.integer("revision",i);event.integer("start_us",static_cast<int64_t>(i-1)*250000);event.integer("end_us",static_cast<int64_t>(i)*250000);
        TEST_ASSERT_EQUAL(DiagnosticRecordResult::Accepted,buffer.diagnostics().record({DiagnosticSourceKind::Sensor,1},event));
        buffer.service(transport,identity());pollfd readable{receiver,POLLIN,0};TEST_ASSERT_EQUAL_INT(1,poll(&readable,1,1000));char wire[UdpLogMaxBytes];int n=recv(receiver,wire,sizeof(wire)-1,0);TEST_ASSERT_TRUE(n>0);wire[n]=0;
        char expected[64];snprintf(expected,sizeof(expected),"\"revision\":%u",i);TEST_ASSERT_NOT_NULL(strstr(wire,expected));
    }
    TEST_ASSERT_EQUAL_UINT(14400,buffer.diagnosticStats().sent);TEST_ASSERT_EQUAL_UINT(0,buffer.diagnosticStats().failed);
    close(receiver);close(transport.fd);
}
int main(){UNITY_BEGIN();RUN_TEST(test_real_udp_loopback_preserves_all_14400_rain_windows);RUN_TEST(test_format_preserves_typed_fields_and_identity);RUN_TEST(test_maximum_existing_events_are_not_truncated);RUN_TEST(test_full_queues_preserve_sequence_gaps_and_fairness);RUN_TEST(test_offline_backpressure_and_errors_discard_without_retry);RUN_TEST(test_one_hour_full_rate_and_controller_bursts);return UNITY_END();}
