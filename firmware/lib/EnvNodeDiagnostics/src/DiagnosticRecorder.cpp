// EnvNode: see repository LICENSE.
#include "BufferedDiagnosticRecorder.h"
#include <cstring>
#include <cmath>
#include <cstdio>
#include <new>
#include <cstdarg>
namespace EnvNode {
namespace {
bool nameValid(const char* text,size_t capacity) {
    if(!text || !text[0]) return false;
    for(size_t i=0;i<capacity;++i) {
        const unsigned char c=text[i];if(!c)return true;
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.')) return false;
    }return false;
}
bool textValid(const char* text) {
    if(!text)return false;size_t n=0;while(n<DiagnosticTextBytes&&text[n])++n;
    if(n==DiagnosticTextBytes)return false;
    for(size_t i=0;i<n;) {
        unsigned char c=text[i++];if(c<128)continue;
        unsigned more=0;uint32_t cp=0,min=0;
        if(c>=0xc2&&c<=0xdf){more=1;cp=c&31;min=128;}
        else if(c>=0xe0&&c<=0xef){more=2;cp=c&15;min=2048;}
        else if(c>=0xf0&&c<=0xf4){more=3;cp=c&7;min=65536;}else return false;
        if(i+more>n)return false;
        while(more--){const unsigned char t=text[i++];if((t&192)!=128)return false;cp=(cp<<6)|(t&63);}
        if(cp<min||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return false;
    }return true;
}
std::atomic<IDiagnosticRecorder*> installed{nullptr};
class RecorderFacade : public IDiagnosticRecorder {
    bool enabled(DiagnosticSource source) const override {auto* sink=installed.load();return sink && sink->enabled(source);}
    DiagnosticRecordResult record(DiagnosticSource source,const DiagnosticEvent& event) override {auto* sink=installed.load();return sink?sink->record(source,event):DiagnosticRecordResult::Unavailable;}
};
RecorderFacade facade;
}
DiagnosticEvent::DiagnosticEvent(const char* name) {valid_=nameValid(name,sizeof(name_));if(valid_)strcpy(name_,name);}
DiagnosticField* DiagnosticEvent::add(const char* key,DiagnosticValueKind kind) {
    if(!valid_||count_==DiagnosticMaxFields||!nameValid(key,DiagnosticKeyBytes)){valid_=false;return nullptr;}
    for(size_t i=0;i<count_;++i)if(!strcmp(fields_[i].key,key)){valid_=false;return nullptr;}
    auto& field=fields_[count_++];strcpy(field.key,key);field.kind=kind;return &field;
}
bool DiagnosticEvent::number(const char* key,double value) {if(!std::isfinite(value)){valid_=false;return false;}auto* f=add(key,DiagnosticValueKind::Number);if(f)f->number=value;return f;}
bool DiagnosticEvent::integer(const char* key,int64_t value) {auto* f=add(key,DiagnosticValueKind::Integer);if(f)f->integer=value;return f;}
bool DiagnosticEvent::boolean(const char* key,bool value) {auto* f=add(key,DiagnosticValueKind::Boolean);if(f)f->boolean=value;return f;}
bool DiagnosticEvent::text(const char* key,const char* value) {if(!textValid(value)){valid_=false;return false;}auto* f=add(key,DiagnosticValueKind::Text);if(f)strcpy(f->text,value);return f;}
IDiagnosticRecorder& diagnosticRecorder() {return facade;}
void installDiagnosticRecorder(IDiagnosticRecorder* recorder) {installed.store(recorder==&facade?nullptr:recorder);}
const char* diagnosticSourceKindName(DiagnosticSourceKind kind) {
    switch(kind){case DiagnosticSourceKind::System:return "system";case DiagnosticSourceKind::Sensor:return "sensor";case DiagnosticSourceKind::Actuator:return "actuator";case DiagnosticSourceKind::Controller:return "controller";case DiagnosticSourceKind::Module:return "module";default:return "invalid";}
}
BufferedDiagnosticRecorder::BufferedDiagnosticRecorder(ILogTimeProvider& clock):clock_(clock),records_(new(std::nothrow) DiagnosticRecord[DiagnosticQueueEntries]) {for(auto& mask:masks_)mask.store(0);}
void BufferedDiagnosticRecorder::setMask(DiagnosticSourceKind kind,uint32_t mask) {const auto k=static_cast<unsigned>(kind);if(k<static_cast<unsigned>(DiagnosticSourceKind::Count)&&masks_[k].exchange(mask)!=mask)++generation_;}
bool BufferedDiagnosticRecorder::selected(DiagnosticSource s) const {
    const auto k=static_cast<unsigned>(s.kind);if(k>=static_cast<unsigned>(DiagnosticSourceKind::Count))return false;
    if(s.kind==DiagnosticSourceKind::System)return s.id==0&&(masks_[k].load()&1);
    return s.id>=1&&s.id<=32&&(masks_[k].load()&(UINT32_C(1)<<(s.id-1)));
}
bool BufferedDiagnosticRecorder::enabled(DiagnosticSource s) const {return available()&&selected(s);}
DiagnosticRecordResult BufferedDiagnosticRecorder::record(DiagnosticSource source,const DiagnosticEvent& event) {
    if(!event.valid()){++invalid_;return DiagnosticRecordResult::Invalid;}
    const auto kind=static_cast<unsigned>(source.kind);
    if(kind>=static_cast<unsigned>(DiagnosticSourceKind::Count)||(source.kind==DiagnosticSourceKind::System?source.id!=0:source.id<1||source.id>32)){++invalid_;return DiagnosticRecordResult::Invalid;}
    if(!selected(source))return DiagnosticRecordResult::Disabled;
    const uint32_t sequence=sequence_.fetch_add(1)+1;
    if(!available()){++unavailable_;return DiagnosticRecordResult::Unavailable;}
    if(lock_.test_and_set(std::memory_order_acquire)){++busy_;return DiagnosticRecordResult::Busy;}
    if(!available()) {++unavailable_;lock_.clear(std::memory_order_release);return DiagnosticRecordResult::Unavailable;}
    const size_t count=count_.load();
    if(count==DiagnosticQueueEntries){++full_;lock_.clear(std::memory_order_release);return DiagnosticRecordResult::Full;}
    auto& r=records_[(head_+count)%DiagnosticQueueEntries];r.source=source;r.event=event;r.sequence=sequence;
    r.uptimeMs=clock_.monotonicMs();r.wallClockValid=clock_.wallClockEpochSeconds(r.epochSeconds);r.generation=generation_.load();
    count_.store(count+1);if(count+1>highWater_.load())highWater_.store(count+1);++accepted_;lock_.clear(std::memory_order_release);return DiagnosticRecordResult::Accepted;
}
bool BufferedDiagnosticRecorder::pop(DiagnosticRecord& record) {
    if(lock_.test_and_set(std::memory_order_acquire))return false;
    bool found=false;
    while(count_.load()) {
        const auto& r=records_[head_];const bool selectedNow=r.generation==generation_.load()&&selected(r.source);
        if(selectedNow){record=r;found=true;}else ++stale_;
        head_=(head_+1)%DiagnosticQueueEntries;count_.fetch_sub(1);if(found)break;
    }
    lock_.clear(std::memory_order_release);return found;
}
bool BufferedDiagnosticRecorder::empty() {
    if(lock_.test_and_set(std::memory_order_acquire))return false;
    const bool result=count_.load()==0;lock_.clear(std::memory_order_release);return result;
}
bool BufferedDiagnosticRecorder::discardPending() {
    if(lock_.test_and_set(std::memory_order_acquire))return false;
    stale_.fetch_add(count_.load());count_.store(0);head_=0;
    lock_.clear(std::memory_order_release);return true;
}
DiagnosticRecorderStats BufferedDiagnosticRecorder::stats() const {DiagnosticRecorderStats s;s.accepted=accepted_;s.full=full_;s.busy=busy_;s.unavailable=unavailable_;s.invalid=invalid_;s.stale=stale_;s.queued=count_;s.highWater=highWater_;return s;}
namespace {
class Json {
public:
    Json(char* output,size_t capacity):out(output),cap(capacity){if(cap)out[0]=0;}
    void text(const char* value) {add("\"");for(const unsigned char* p=reinterpret_cast<const unsigned char*>(value);*p;++p){if(*p=='"'||*p=='\\')add("\\%c",*p);else if(*p<32||*p==127)add("\\u%04x",*p);else add("%c",*p);}add("\"");}
    void add(const char* format,...) {if(!ok)return;va_list args;va_start(args,format);const int n=vsnprintf(out+pos,cap-pos,format,args);va_end(args);if(n<0||static_cast<size_t>(n)>=cap-pos){ok=false;if(cap)out[0]=0;return;}pos+=n;}
    char* out;size_t cap,pos=0;bool ok=true;
};
}
size_t encodeDiagnosticRecord(const DiagnosticRecord& r,char* output,size_t capacity) {
    if(!output||!capacity||!r.event.valid())return 0;Json j(output,capacity);
    j.add("{\"type\":\"diagnostic\",\"schema_version\":1,\"sequence\":%lu,\"uptime_ms\":%lu,\"epoch_s\":",static_cast<unsigned long>(r.sequence),static_cast<unsigned long>(r.uptimeMs));
    if(r.wallClockValid)j.add("%lld",static_cast<long long>(r.epochSeconds));else j.add("null");
    j.add(",\"source_kind\":");j.text(diagnosticSourceKindName(r.source.kind));j.add(",\"source_id\":%u,\"event\":",static_cast<unsigned>(r.source.id));j.text(r.event.name());j.add(",\"values\":{");
    for(size_t i=0;i<r.event.count();++i){if(i)j.add(",");const auto& f=r.event.field(i);j.text(f.key);j.add(":");switch(f.kind){case DiagnosticValueKind::Number:j.add("%.17g",f.number);break;case DiagnosticValueKind::Integer:j.add("%lld",static_cast<long long>(f.integer));break;case DiagnosticValueKind::Boolean:j.add(f.boolean?"true":"false");break;case DiagnosticValueKind::Text:j.text(f.text);break;}}
    j.add("}}\n");return j.ok?j.pos:0;
}
}
