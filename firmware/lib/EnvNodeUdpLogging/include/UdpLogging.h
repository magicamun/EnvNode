// EnvNode-owned: see repository LICENSE.
#pragma once
#include "BufferedDiagnosticRecorder.h"
#include "LogEntry.h"
#include <atomic>
#include <memory>
namespace EnvNode {
// Preserve the entire existing event contract, including worst-case JSON escaping.
// Normal rain records fit a single Ethernet datagram; maximum events may fragment.
constexpr size_t UdpLogMaxBytes=2048;
constexpr size_t UdpSystemQueueEntries=8;
struct UdpLogIdentity {
    char device[13]={}; // 48-bit factory identity, lower-case hex
    char boot[33]={};   // 128-bit random boot identity, lower-case hex
    char build[96]={};
};
size_t encodeSyslogDiagnostic(const UdpLogIdentity&,const DiagnosticRecord&,char*,size_t);
size_t encodeSyslogSystem(const UdpLogIdentity&,const LogEntry&,char*,size_t);
enum class UdpSendResult {Sent,Offline,WouldBlock,Failed};
class IUdpLogTransport {
public:
    virtual ~IUdpLogTransport()=default;
    // Must not wait, allocate per event or perform DNS; Sent is local acceptance only.
    virtual UdpSendResult send(const char* bytes,size_t count)=0;
};
struct UdpStreamStats {
    uint32_t accepted=0,full=0,busy=0,sent=0,offline=0,wouldBlock=0,failed=0,encodeError=0,stale=0;
    size_t queued=0,highWater=0;
};
// Construct after runtime heap is available. No task/heap allocation during service.
// Producer admission and transport availability are deliberately independent.
class UdpLogBuffer {
public:
    explicit UdpLogBuffer(ILogTimeProvider& clock);
    bool available() const;
    BufferedDiagnosticRecorder& diagnostics(){return diagnostics_;}
    bool recordSystem(const LogEntry&);
    void setSystemEnabled(bool enabled){systemEnabled_.store(enabled);}
    // At most one event from each stream; errors are counted and discarded.
    void service(IUdpLogTransport&,const UdpLogIdentity&);
    UdpStreamStats systemStats() const;
    UdpStreamStats diagnosticStats() const;
private:
    struct Counters {
        std::atomic<uint32_t> accepted{0},full{0},busy{0},sent{0},offline{0},wouldBlock{0},failed{0},encodeError{0},stale{0};
        std::atomic<size_t> highWater{0};
        void result(UdpSendResult);
        UdpStreamStats snapshot() const;
    } system_,diagnostic_;
    BufferedDiagnosticRecorder diagnostics_;
    std::unique_ptr<LogEntry[]> entries_;
    std::unique_ptr<char[]> wire_;
    std::atomic<bool> systemEnabled_{false};
    std::atomic_flag lock_=ATOMIC_FLAG_INIT;
    size_t head_=0;
    std::atomic<size_t> count_{0};
};
}
