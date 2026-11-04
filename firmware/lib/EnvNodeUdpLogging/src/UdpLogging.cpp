// EnvNode-owned: see repository LICENSE.
#include "UdpLogging.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <new>
namespace EnvNode {
namespace {
class Writer {
public:
    Writer(char* bytes,size_t capacity):out(bytes),cap(capacity){if(cap)out[0]=0;}
    void add(const char* format,...) {
        if(!ok)return;
        va_list args;va_start(args,format);
        const int n=vsnprintf(out+pos,cap-pos,format,args);va_end(args);
        if(n<0||static_cast<size_t>(n)>=cap-pos){ok=false;if(cap)out[0]=0;return;}
        pos+=n;
    }
    void text(const char* value) {
        add("\"");
        for(const unsigned char* p=reinterpret_cast<const unsigned char*>(value);*p;++p) {
            if(*p=='"'||*p=='\\')add("\\%c",*p);
            else if(*p<32||*p==127)add("\\u%04x",*p);
            else add("%c",*p);
        }
        add("\"");
    }
    size_t size() const{return ok?pos:0;}
    char* out;size_t cap,pos=0;bool ok=true;
};
bool hexId(const char* s,size_t n) {
    for(size_t i=0;i<n;++i)if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return false;
    return s[n]==0;
}
bool validIdentity(const UdpLogIdentity& id) {
    if(!hexId(id.device,12)||!hexId(id.boot,32)||!memchr(id.build,0,sizeof(id.build)))return false;
    for(const char* p=id.build;*p;++p)if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||strchr(".+_-",*p)))return false;
    return id.build[0]!=0;
}
void metadata(Writer& w,const UdpLogIdentity& id) {
    w.add(",\"device_id\":");w.text(id.device);w.add(",\"boot_id\":");w.text(id.boot);
    w.add(",\"build\":");w.text(id.build);
}
void header(Writer& w,const UdpLogIdentity& id,bool diagnostic,LogLevel level=LogLevel::Info) {
    // local0 facility, RFC5424 version 1; UTC stays in the original JSON field.
    unsigned severity=level==LogLevel::Error?3:level==LogLevel::Warn?4:level==LogLevel::Debug?7:6;
    w.add("<%u>1 - %s %s %s - - ",128+severity,id.device,
        diagnostic?"envnode-diagnostic":"envnode-system",id.boot);
}
}
size_t encodeSyslogDiagnostic(const UdpLogIdentity& id,const DiagnosticRecord& record,char* out,size_t cap) {
    if(!out||!cap)return 0;out[0]=0;if(!validIdentity(id))return 0;
    Writer w(out,cap);header(w,id,true);if(!w.ok)return 0;
    size_t n=encodeDiagnosticRecord(record,out+w.pos,cap-w.pos);
    if(n<3){out[0]=0;return 0;}
    // Replace the final object close/newline; keep the existing typed event intact.
    w.pos+=n-2;
    w.add(",\"selection_generation\":%lu",static_cast<unsigned long>(record.generation));
    metadata(w,id);w.add("}");return w.size();
}
size_t encodeSyslogSystem(const UdpLogIdentity& id,const LogEntry& entry,char* out,size_t cap) {
    if(!out||!cap)return 0;out[0]=0;if(!validIdentity(id))return 0;
    Writer w(out,cap);header(w,id,false,entry.level);
    w.add("{\"type\":\"system\",\"schema_version\":1,\"sequence\":%lu,\"uptime_ms\":%lu,\"epoch_s\":",
        static_cast<unsigned long>(entry.sequence),static_cast<unsigned long>(entry.monotonicMs));
    if(entry.wallClockValid)w.add("%lld",static_cast<long long>(entry.epochSeconds));else w.add("null");
    w.add(",\"level\":");w.text(logLevelDisplayName(entry.level));w.add(",\"message\":");w.text(entry.message);
    metadata(w,id);w.add("}");return w.size();
}
UdpLogBuffer::UdpLogBuffer(ILogTimeProvider& clock):diagnostics_(clock),
    entries_(new(std::nothrow) LogEntry[UdpSystemQueueEntries]),wire_(new(std::nothrow) char[UdpLogMaxBytes]){diagnostics_.setAvailable(true);}
bool UdpLogBuffer::available() const{return entries_&&wire_&&diagnostics_.available();}
bool UdpLogBuffer::recordSystem(const LogEntry& entry) {
    if(!systemEnabled_.load()||!entries_)return false;
    if(lock_.test_and_set(std::memory_order_acquire)){++system_.busy;return false;}
    const size_t count=count_.load();
    if(count==UdpSystemQueueEntries){++system_.full;lock_.clear(std::memory_order_release);return false;}
    entries_[(head_+count)%UdpSystemQueueEntries]=entry;count_.store(count+1);++system_.accepted;
    if(count+1>system_.highWater.load())system_.highWater.store(count+1);
    lock_.clear(std::memory_order_release);return true;
}
void UdpLogBuffer::Counters::result(UdpSendResult value) {
    switch(value){case UdpSendResult::Sent:++sent;break;case UdpSendResult::Offline:++offline;break;
        case UdpSendResult::WouldBlock:++wouldBlock;break;case UdpSendResult::Failed:++failed;break;}
}
UdpStreamStats UdpLogBuffer::Counters::snapshot() const {
    UdpStreamStats s;s.accepted=accepted;s.full=full;s.busy=busy;s.sent=sent;s.offline=offline;
    s.wouldBlock=wouldBlock;s.failed=failed;s.encodeError=encodeError;s.stale=stale;s.highWater=highWater;return s;
}
UdpStreamStats UdpLogBuffer::systemStats() const {auto s=system_.snapshot();s.queued=count_;return s;}
UdpStreamStats UdpLogBuffer::diagnosticStats() const {
    auto s=diagnostic_.snapshot();const auto d=diagnostics_.stats();s.accepted=d.accepted;s.full=d.full;s.busy=d.busy;
    s.queued=d.queued;s.highWater=d.highWater;return s;
}
void UdpLogBuffer::service(IUdpLogTransport& transport,const UdpLogIdentity& identity) {
    if(!wire_)return;
    LogEntry entry;bool found=false;
    if(!lock_.test_and_set(std::memory_order_acquire)) {
        if(count_.load()){entry=entries_[head_];head_=(head_+1)%UdpSystemQueueEntries;count_.fetch_sub(1);found=true;}
        lock_.clear(std::memory_order_release);
    }
    if(found) {
        if(systemEnabled_.load()) {
            const size_t n=encodeSyslogSystem(identity,entry,wire_.get(),UdpLogMaxBytes);
            if(n)system_.result(transport.send(wire_.get(),n));else ++system_.encodeError;
        } else ++system_.stale;
    }
    DiagnosticRecord record;
    if(diagnostics_.pop(record)) {
        const size_t n=encodeSyslogDiagnostic(identity,record,wire_.get(),UdpLogMaxBytes);
        if(n)diagnostic_.result(transport.send(wire_.get(),n));else ++diagnostic_.encodeError;
    }
}
}
