// EnvNode: see repository LICENSE.
#pragma once
#include "IDiagnosticRecorder.h"
#include "ILogTimeProvider.h"
#include <atomic>
#include <memory>
namespace EnvNode {
constexpr size_t DiagnosticQueueEntries=8;
struct DiagnosticRecord {
    DiagnosticSource source;DiagnosticEvent event{"invalid"};
    uint32_t sequence=0,uptimeMs=0,generation=0;int64_t epochSeconds=0;bool wallClockValid=false,explicitMarker=false;
};
static_assert(sizeof(DiagnosticRecord)*DiagnosticQueueEntries<=8192,"bounded diagnostic queue memory budget");
struct DiagnosticRecorderStats {uint32_t accepted=0,full=0,busy=0,unavailable=0,invalid=0,stale=0;size_t queued=0,highWater=0;};
class BufferedDiagnosticRecorder : public IDiagnosticRecorder {
public:
    explicit BufferedDiagnosticRecorder(ILogTimeProvider& clock);
    void setMask(DiagnosticSourceKind kind,uint32_t mask);
    void setAvailable(bool available) {available_.store(available);}
    bool available() const {return available_.load() && records_!=nullptr;}
    bool enabled(DiagnosticSource source) const override;
    DiagnosticRecordResult record(DiagnosticSource source,const DiagnosticEvent& event) override;
    // Explicit operator action: same bounded queue/sequence, independent of masks.
    // Only the test_marker event can use this path.
    DiagnosticRecordResult recordMarker(const DiagnosticEvent& event);
    bool pop(DiagnosticRecord& record);
    bool empty(); // synchronized with producers, including admission shutdown
    bool discardPending();
    uint32_t generation() const {return generation_.load();}
    DiagnosticRecorderStats stats() const;
private:
    bool selected(DiagnosticSource source) const;
    DiagnosticRecordResult recordImpl(DiagnosticSource,const DiagnosticEvent&,bool explicitMarker);
    ILogTimeProvider& clock_;std::unique_ptr<DiagnosticRecord[]> records_;
    std::atomic<uint32_t> masks_[static_cast<unsigned>(DiagnosticSourceKind::Count)];
    std::atomic<bool> available_{false};std::atomic_flag lock_=ATOMIC_FLAG_INIT;
    std::atomic<size_t> count_{0};size_t head_=0;std::atomic<uint32_t> sequence_{0};std::atomic<size_t> highWater_{0};
    std::atomic<uint32_t> generation_{0},accepted_{0},full_{0},busy_{0},unavailable_{0},invalid_{0},stale_{0};
};
size_t encodeDiagnosticRecord(const DiagnosticRecord& record,char* output,size_t capacity);
}
