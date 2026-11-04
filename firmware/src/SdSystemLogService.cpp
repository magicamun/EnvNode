// EnvNode: see repository LICENSE.
#include "SdSystemLogService.h"
#include "SdSystemLog.h"
#include "BufferedDiagnosticRecorder.h"
#include "SdCardSpiDiagnostic.h"
#include "SdStorageArbiter.h"
#include <Preferences.h>
#include <esp_heap_caps.h>
#include <atomic>
#include <new>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
namespace EnvNode {
namespace {
struct RecordingService {
    SdLogQueue queue;
    SdLogBatch batch;
    explicit RecordingService(ILogTimeProvider& clock):diagnostics(clock) {}
    BufferedDiagnosticRecorder diagnostics;
    SdLogBatch diagnosticBatch;
    std::unique_ptr<ISdLogStorage> diagnosticStorage;
    DiagnosticRecord pendingDiagnostic;
    bool hasPendingDiagnostic=false;
    unsigned diagnosticMode=0;uint32_t diagnosticGeneration=0,lastDropped=0;
    std::atomic<unsigned> diagnosticStatus{0};
    std::atomic<uint32_t> stackFreeBytes{0};
    void serviceDiagnostics(unsigned mode,bool draining=false) {
        const uint32_t generation=diagnostics.generation();
        if(diagnosticMode!=(mode&6) || diagnosticGeneration!=generation) {
            diagnosticStorage.reset();diagnosticBatch.clear();hasPendingDiagnostic=false;
            diagnosticMode=mode&6;diagnosticGeneration=generation;diagnosticStatus.store((mode&4)?1:0);
        }
        if(!(mode&4)) return;
        if(!diagnosticStorage) diagnosticStorage=createSdDiagnosticStorage((mode&2)?ModuleSlot::B:ModuleSlot::A,session);
        if(!diagnosticStorage){diagnosticStatus.store(3);return;}
        const uint32_t now=millis();const auto stats=diagnostics.stats();
        const uint32_t dropped=stats.full+stats.busy+stats.unavailable+stats.stale;
        if(dropped!=lastDropped) {
            char line[160];const int n=snprintf(line,sizeof(line),"{\"type\":\"diagnostic_loss\",\"schema_version\":1,\"count\":%lu,\"uptime_ms\":%lu}\n",static_cast<unsigned long>(dropped-lastDropped),static_cast<unsigned long>(now));
            if(n>0 && diagnosticBatch.addEncoded(line,n,now))lastDropped=dropped;
        }
        bool force=false;
        for(unsigned i=0;i<4;++i) {
            if(!hasPendingDiagnostic) hasPendingDiagnostic=diagnostics.pop(pendingDiagnostic);
            if(!hasPendingDiagnostic)break;
            char line[2048];const size_t length=encodeDiagnosticRecord(pendingDiagnostic,line,sizeof(line));
            if(!length){hasPendingDiagnostic=false;diagnosticStatus.store(3);continue;}
            if(!diagnosticBatch.addEncoded(line,length,now)){force=true;break;}
            hasPendingDiagnostic=false;
        }
        // A settings change may revoke a source while the worker was encoding it.
        if(diagnostics.generation()!=generation){diagnosticBatch.clear();hasPendingDiagnostic=false;return;}
        const auto result=diagnosticBatch.flush(*diagnosticStorage,now,force||draining);
        if(result==SdLogWriteResult::Written)diagnosticStatus.store(2);
        else if(result==SdLogWriteResult::Failed)diagnosticStatus.store(3);
    }

    portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
    std::atomic<unsigned> mode{0};
    std::atomic<unsigned> status{0}; // 0 disabled, 1 waiting, 2 recording, 3 error.
    uint32_t session=0;
    bool pop(LogEntry& entry) {
        portENTER_CRITICAL(&mux);const bool found=queue.pop(entry);portEXIT_CRITICAL(&mux);return found;
    }
    uint32_t lost() {
        portENTER_CRITICAL(&mux);const uint32_t count=queue.takeLost();portEXIT_CRITICAL(&mux);return count;
    }
    void clearQueue() {portENTER_CRITICAL(&mux);queue.clear();portEXIT_CRITICAL(&mux);}
    static void worker(void* context) {
        auto& self=*static_cast<RecordingService*>(context);
        unsigned previous=self.mode.load();
        std::unique_ptr<ISdLogStorage> storage;
        LogEntry pending;bool hasPending=false;uint32_t lost=0,lastStackProbe=0,mediaGeneration=0;
        for(;;) {
            serviceSdCardMount(false);
            const unsigned mode=self.mode.load();
            const uint32_t generation=sdCardMediaGeneration();
            if(generation!=mediaGeneration) {
                // Never carry data from a removed/changed medium onto the next one.
                if(!self.diagnostics.discardPending()) {vTaskDelay(pdMS_TO_TICKS(20));continue;}
                storage.reset();self.diagnosticStorage.reset();self.batch.clear();self.diagnosticBatch.clear();
                self.clearQueue();hasPending=self.hasPendingDiagnostic=false;lost=0;
                mediaGeneration=generation;
            }
            const bool draining=sdCardEjectRequested();
            if((mode&3)!=(previous&3)) {
                storage.reset();self.batch.clear();self.clearQueue();hasPending=false;lost=0;previous=mode;
                if(mode&1) {
                    LogEntry started;started.monotonicMs=millis();
                    strcpy(started.message,"SD system log recording started");
                    self.batch.add(started,started.monotonicMs);
                }
            }
            if(mode&1) {
                if(!storage) storage=createSdSystemLogStorage((mode&2) ? ModuleSlot::B : ModuleSlot::A,self.session);
                if(!storage) self.status.store(3);
                else {
                    const uint32_t now=millis();lost+=self.lost();
                    if(lost && self.batch.addLoss(lost,now)) lost=0;
                    bool force=false;
                    for(unsigned count=0;count<8;++count) {
                        if(!hasPending) hasPending=self.pop(pending);
                        if(!hasPending) break;
                        if(!self.batch.add(pending,now)) {force=true;break;}
                        hasPending=false;
                    }
                    const auto result=self.batch.flush(*storage,now,force||draining);
                    if(result==SdLogWriteResult::Written) self.status.store(2);
                    else if(result==SdLogWriteResult::Failed) self.status.store(3);
                }
            } else self.status.store(0);
            self.serviceDiagnostics(mode,draining);
            bool emptyQueue;
            portENTER_CRITICAL(&self.mux);emptyQueue=self.queue.count()==0;portEXIT_CRITICAL(&self.mux);
            const bool drained=emptyQueue && !hasPending && !lost && !self.batch.bytes()
                && self.diagnostics.empty() && !self.hasPendingDiagnostic && !self.diagnosticBatch.bytes();
            serviceSdCardMount(drained);
            if(millis()-lastStackProbe>=1000){self.stackFreeBytes.store(uxTaskGetStackHighWaterMark(nullptr));lastStackProbe=millis();}
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
};
RecordingService* recording=nullptr;
unsigned lastReported=0;
}
void ejectSdCardRecording() {
    // Close diagnostic admission before publishing the drain request to the worker.
    if(recording) recording->diagnostics.setAvailable(false);
    requestSdCardEject();
}
void enqueueSdSystemLog(const LogEntry& entry) {
    if(!recording || !(recording->mode.load()&1) || sdCardEjectRequested()) return;
    portENTER_CRITICAL(&recording->mux);if(!sdCardEjectRequested())recording->queue.push(entry);portEXIT_CRITICAL(&recording->mux);
}
bool sdSystemLogSlotEnabled(ModuleSlot slot) {
    if(!recording) return false;
    const unsigned mode=recording->mode.load();
    return (mode&1) && ((mode&2) ? ModuleSlot::B : ModuleSlot::A)==slot;
}
void configureSdSystemLog(const Configuration& configuration,const ActuatorRuntime& actuators) {
    if(!recording) return;
    const bool access=recording->session && configuration.sdStorage.enabled
        && sdStoragePinsAvailable(configuration,actuators) && sdRuntimeStable();
    configureSdCardMount(access,configuration.sdStorage.slot);
    recording->diagnostics.setAvailable(access && sdDiagnosticCardPresent(configuration));
    uint32_t sensors=0,actors=0,controllers=0;
    static_assert(MaxSensorSlotCount<=32 && MaxActuatorSlotCount<=32 && MaxControllerSlotCount<=32,"diagnostic source mask capacity");
    for(size_t i=0;i<MaxSensorSlotCount;++i)if(configuration.sensorSlots[i].enabled && configuration.sensorSlots[i].extendedDiagnostics)sensors|=UINT32_C(1)<<i;
    for(size_t i=0;i<MaxActuatorSlotCount;++i)if(configuration.actuatorSlots[i].enabled && configuration.actuatorSlots[i].extendedDiagnostics)actors|=UINT32_C(1)<<i;
    for(size_t i=0;i<MaxControllerSlotCount;++i)if(configuration.controllerSlots[i].enabled && configuration.controllerSlots[i].extendedDiagnostics)controllers|=UINT32_C(1)<<i;
    recording->diagnostics.setMask(DiagnosticSourceKind::System,configuration.sdStorage.systemDiagnostics?1:0);
    recording->diagnostics.setMask(DiagnosticSourceKind::Module,configuration.sdStorage.systemDiagnostics?3:0);
    recording->diagnostics.setMask(DiagnosticSourceKind::Sensor,sensors);
    recording->diagnostics.setMask(DiagnosticSourceKind::Actuator,actors);
    recording->diagnostics.setMask(DiagnosticSourceKind::Controller,controllers);
    unsigned mode=configuration.sdStorage.slot==ModuleSlot::B?2:0;
    if(access && configuration.sdStorage.systemLog)mode|=1;
    if(access && (configuration.sdStorage.systemDiagnostics || sensors || actors || controllers))mode|=4;
    const unsigned old=recording->mode.exchange(mode);
    if((mode&3)!=(old&3)) recording->status.store((mode&1)?1:0);

}
void beginSdSystemLog(const Configuration& configuration,const ActuatorRuntime& actuators,const RecentLogStore& recent,ILogger& logger,ILogTimeProvider& clock) {
    if(recording) return;
    std::unique_ptr<RecordingService> candidate(new(std::nothrow) RecordingService(clock));
    if(!candidate || !candidate->queue.available()) {logger.warn("SD log: RAM buffer unavailable; recording disabled");return;}
    Preferences metadata;
    if(metadata.begin("sd_log_meta",false)) {
        const uint32_t previous=metadata.getUInt("boot",0);
        if(previous!=UINT32_MAX && metadata.putUInt("boot",previous+1)==sizeof(uint32_t)) candidate->session=previous+1;
        metadata.end();
    }
    if(!candidate->session) {logger.warn("SD log: session counter unavailable; recording disabled");return;}
    recording=candidate.release();
    configureSdSystemLog(configuration,actuators);
    // Capture recent startup entries before the worker starts; queue overflow is counted.
    if(recording->mode.load()&1) for(size_t i=0;i<recent.count();++i) {
        LogEntry entry;if(recent.copyEntry(i,entry)) enqueueSdSystemLog(entry);
    }
    TaskHandle_t task=nullptr;
    if(xTaskCreatePinnedToCore(RecordingService::worker,"sd-log",12288,recording,1,&task,1)!=pdPASS) {
        delete recording;recording=nullptr;logger.warn("SD log: writer task unavailable; recording disabled");return;
    }
    installDiagnosticRecorder(&recording->diagnostics);
}
void serviceSdSystemLog(const Configuration& configuration,const ActuatorRuntime& actuators,ILogger& logger) {
    configureSdSystemLog(configuration,actuators);
    if(!recording) return;
    static const char* lastMountStatus=nullptr;
    const char* mountStatus=sdCardMountStatus();
    if(mountStatus!=lastMountStatus) {logger.infof("SD card: %s",mountStatus);lastMountStatus=mountStatus;}
    static unsigned lastDiagnosticStatus=0;
    const unsigned diagnosticStatus=recording->diagnosticStatus.load();
    if(diagnosticStatus!=lastDiagnosticStatus) {
        if(diagnosticStatus==3)logger.warn("Extended diagnostics: write failed; buffered retry in 30 seconds");
        else if(diagnosticStatus==2) {
            logger.info("Extended diagnostics: recording active");
            logger.infof("SD writer resources: heap8=%u largest8=%u stack_min_bytes=%u",
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)),
                static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)),
                static_cast<unsigned>(recording->stackFreeBytes.load()));
        }
        lastDiagnosticStatus=diagnosticStatus;
    }
    const unsigned status=recording->status.load();
    if(status!=lastReported) {
        if(status==3) logger.warn("SD log: write failed; buffered retry in 30 seconds (check card/free space)");
        else if(status==2) logger.info("SD log: recording active");
        lastReported=status;
    }
}
String sdSystemLogStatus() {
    if(!recording) return "unavailable";
    size_t queued;uint32_t dropped;
    portENTER_CRITICAL(&recording->mux);queued=recording->queue.count();dropped=recording->queue.dropped();portEXIT_CRITICAL(&recording->mux);
    const char* states[]={"disabled","waiting","recording","write error / retry"};
    const unsigned status=recording->status.load();
    return String(sdCardEjectRequested()?"paused (card eject)":status<4 ? states[status] : "unknown")+" · queue="+String(static_cast<unsigned>(queued))+"/28 · dropped="+String(dropped);
}
bool sdDiagnosticSlotEnabled(ModuleSlot slot) {
    if(!recording)return false;const unsigned mode=recording->mode.load();
    return (mode&4) && ((mode&2)?ModuleSlot::B:ModuleSlot::A)==slot;
}
bool extendedDiagnosticsAvailable(){return recording && recording->diagnostics.available();}
String extendedDiagnosticsStatus(){
    if(!recording)return "unavailable";const auto s=recording->diagnostics.stats();
    const char* states[]={"disabled","waiting","recording","write error / retry"};const unsigned status=recording->diagnosticStatus.load();
    return String(sdCardEjectRequested()?"paused (card eject)":status<4?states[status]:"unknown")+" · queued="+String(static_cast<unsigned>(s.queued))+"/8 · accepted="+String(s.accepted)+" · full="+String(s.full)+" · busy="+String(s.busy)+" · unavailable="+String(s.unavailable)+" · invalid="+String(s.invalid)+" · stale="+String(s.stale)+" · writer_stack_min_bytes="+String(recording->stackFreeBytes.load());
}


}
