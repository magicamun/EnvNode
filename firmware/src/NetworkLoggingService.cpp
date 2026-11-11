// EnvNode-owned: see repository LICENSE.
#include "NetworkLoggingService.h"
#include "UdpLogging.h"
#include "FirmwareBuildInfo.h"
#include <WiFi.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <lwip/sockets.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <new>
namespace EnvNode {
namespace {
class SocketTransport:public IUdpLogTransport {
public:
    void configure(const NetworkLoggingConfiguration& c,bool active) {
        if(c.address!=address_||c.port!=port_||active!=enabled_) {
            closeSocket();address_=c.address;port_=c.port;enabled_=active;retryAt_=0;
            memset(&destination_,0,sizeof(destination_));destination_.sin_family=AF_INET;
            destination_.sin_port=htons(port_);inet_pton(AF_INET,address_.c_str(),&destination_.sin_addr);
        }
    }
    UdpSendResult send(const char* bytes,size_t size) override {
        if(!enabled_||WiFi.status()!=WL_CONNECTED){closeSocket();return UdpSendResult::Offline;}
        const uint32_t now=millis();
        if(fd_<0) {
            if(retryAt_&&static_cast<int32_t>(now-retryAt_)<0)return UdpSendResult::WouldBlock;
            fd_=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
            if(fd_<0){lastError=errno;retryAt_=now+1000;return UdpSendResult::Failed;}
            if(fcntl(fd_,F_SETFL,O_NONBLOCK)<0){lastError=errno;closeSocket();retryAt_=now+1000;return UdpSendResult::Failed;}
        }
        const int n=sendto(fd_,bytes,size,MSG_DONTWAIT,reinterpret_cast<const sockaddr*>(&destination_),sizeof(destination_));
        if(n==static_cast<int>(size))return UdpSendResult::Sent;
        lastError=n<0?errno:EMSGSIZE;
        if(lastError==EAGAIN||lastError==EWOULDBLOCK||lastError==ENOBUFS||lastError==ENOMEM)return UdpSendResult::WouldBlock;
        closeSocket();retryAt_=now+1000;return UdpSendResult::Failed;
    }
    int lastError=0;
private:
    void closeSocket(){if(fd_>=0){close(fd_);fd_=-1;}}
    int fd_=-1;bool enabled_=false;String address_;uint16_t port_=0;uint32_t retryAt_=0;sockaddr_in destination_{};
};
std::unique_ptr<UdpLogBuffer> buffer;
SocketTransport transport;
UdpLogIdentity identity;
uint32_t maxServiceUs=0,maxApplicationUs=0;
bool active=false;
TestMarkerSession testMarkers;
void configure(const Configuration& c) {
    if(!buffer)return;
    active=c.networkLogging.enabled&&validNetworkLogging(c.networkLogging);
    transport.configure(c.networkLogging,active);
    buffer->setSystemEnabled(active&&c.networkLogging.systemLog);
    uint32_t sensors=0,actors=0,controllers=0;
    static_assert(MaxSensorSlotCount<=32&&MaxActuatorSlotCount<=32&&MaxControllerSlotCount<=32,"diagnostic mask capacity");
    for(size_t i=0;i<MaxSensorSlotCount;++i)if(c.sensorSlots[i].enabled&&c.sensorSlots[i].extendedDiagnostics)sensors|=UINT32_C(1)<<i;
    for(size_t i=0;i<MaxActuatorSlotCount;++i)if(c.actuatorSlots[i].enabled&&c.actuatorSlots[i].extendedDiagnostics)actors|=UINT32_C(1)<<i;
    for(size_t i=0;i<MaxControllerSlotCount;++i)if(c.controllerSlots[i].enabled&&c.controllerSlots[i].extendedDiagnostics)controllers|=UINT32_C(1)<<i;
    auto& recorder=buffer->diagnostics();recorder.setAvailable(active);
    recorder.setMask(DiagnosticSourceKind::System,c.networkLogging.systemDiagnostics?1:0);
    recorder.setMask(DiagnosticSourceKind::Module,c.networkLogging.systemDiagnostics?3:0);
    recorder.setMask(DiagnosticSourceKind::Sensor,sensors);recorder.setMask(DiagnosticSourceKind::Actuator,actors);recorder.setMask(DiagnosticSourceKind::Controller,controllers);
}
String status(const UdpStreamStats& s) {
    return String("queued=")+String(static_cast<unsigned>(s.queued))+"/8 high_water="+String(static_cast<unsigned>(s.highWater))
        +" accepted="+String(s.accepted)+" sent_local="+String(s.sent)+" full="+String(s.full)+" busy="+String(s.busy)
        +" offline="+String(s.offline)+" would_block="+String(s.wouldBlock)+" send_error="+String(s.failed)+" encode_error="+String(s.encodeError)+" stale="+String(s.stale);
}
}
void beginNetworkLogging(const Configuration& c,ILogTimeProvider& clock,const RecentLogStore& recent) {
    if(buffer)return;
    snprintf(identity.device,sizeof(identity.device),"%012llx",static_cast<unsigned long long>(ESP.getEfuseMac()&UINT64_C(0xffffffffffff)));
    // Fresh random identity on every boot; no flash writes or NVS dependency.
    uint8_t boot[16];esp_fill_random(boot,sizeof(boot));
    for(size_t i=0;i<sizeof(boot);++i)snprintf(identity.boot+2*i,3,"%02x",boot[i]);
    snprintf(identity.build,sizeof(identity.build),"%s",FirmwareBuildInfo::CompactIdentity);
    std::unique_ptr<UdpLogBuffer> candidate(new(std::nothrow) UdpLogBuffer(clock));
    if(!candidate||!candidate->available())return;
    buffer=std::move(candidate);configure(c);installDiagnosticRecorder(&buffer->diagnostics());
    for(size_t i=0;i<recent.count();++i){LogEntry e;if(recent.copyEntry(i,e))buffer->recordSystem(e);}
}
void serviceNetworkLogging(const Configuration& c,uint32_t applicationUs) {
    if(applicationUs>maxApplicationUs)maxApplicationUs=applicationUs;
    if(!buffer)return;const uint32_t start=micros();configure(c);buffer->service(transport,identity);
    const uint32_t elapsed=micros()-start;if(elapsed>maxServiceUs)maxServiceUs=elapsed;
}
const TestMarkerSession& networkTestMarkers(){return testMarkers;}
const char* networkLoggingBootId(){return identity.boot;}
TestMarkerResult recordNetworkTestMarker(const Configuration& c,TestMarkerAction action,
    const char* label,const char* note,uint32_t expectedActiveId) {
    if(!buffer)return TestMarkerResult::Unavailable;
    configure(c);
    if(!active)return TestMarkerResult::Disabled;
    if(WiFi.status()!=WL_CONNECTED)return TestMarkerResult::Unavailable;
    struct Sink : ITestMarkerRecorder {
        BufferedDiagnosticRecorder& recorder;
        explicit Sink(BufferedDiagnosticRecorder& value):recorder(value){}
        DiagnosticRecordResult recordMarker(const DiagnosticEvent& event) override {return recorder.recordMarker(event);}
    } sink(buffer->diagnostics());
    return testMarkers.submit(action,label,note,expectedActiveId,sink);
}
void enqueueNetworkSystemLog(const LogEntry& entry){if(buffer)buffer->recordSystem(entry);}
bool networkLoggingAvailable(){return buffer!=nullptr;}
// Selection is editable even when the receiver/WLAN is unavailable or logging disabled.
bool extendedDiagnosticsAvailable(){return networkLoggingAvailable();}
String networkLoggingStatus(){return buffer?(String(active?"enabled ":"disabled ")+status(buffer->systemStats())+" last_errno="+String(transport.lastError)+" max_service_us="+String(maxServiceUs)+" max_application_us="+String(maxApplicationUs)
        +" heap8="+String(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT))
        +" min_heap8="+String(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT))
        +" largest8="+String(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT))):String("unavailable (RAM allocation failed)");}
String extendedDiagnosticsStatus(){
    if(!buffer)return "unavailable";const auto s=buffer->diagnostics().stats();
    return status(buffer->diagnosticStats())+" unavailable="+String(s.unavailable)+" invalid="+String(s.invalid)+" stale="+String(s.stale);
}
}
