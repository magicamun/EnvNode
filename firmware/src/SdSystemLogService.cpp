// EnvNode: see repository LICENSE.
#include "SdSystemLogService.h"
#include "SdSystemLog.h"
#include "SdCardSpiDiagnostic.h"
#include <Preferences.h>
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
        LogEntry pending;bool hasPending=false;uint32_t lost=0;
        for(;;) {
            const unsigned mode=self.mode.load();
            if(mode!=previous) {
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
                    const auto result=self.batch.flush(*storage,now,force);
                    if(result==SdLogWriteResult::Written) self.status.store(2);
                    else if(result==SdLogWriteResult::Failed) self.status.store(3);
                }
            } else self.status.store(0);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
};
RecordingService* recording=nullptr;
unsigned lastReported=0;
}
void enqueueSdSystemLog(const LogEntry& entry) {
    if(!recording || !(recording->mode.load()&1)) return;
    portENTER_CRITICAL(&recording->mux);recording->queue.push(entry);portEXIT_CRITICAL(&recording->mux);
}
bool sdSystemLogSlotEnabled(ModuleSlot slot) {
    if(!recording) return false;
    const unsigned mode=recording->mode.load();
    return (mode&1) && ((mode&2) ? ModuleSlot::B : ModuleSlot::A)==slot;
}
void configureSdSystemLog(const Configuration& configuration,const ActuatorRuntime& actuators) {
    if(!recording) return;
    unsigned mode=0;
    if(recording->session && configuration.sdStorage.enabled && configuration.sdStorage.systemLog
        && sdStoragePinsAvailable(configuration,actuators))
        mode=1 | (configuration.sdStorage.slot==ModuleSlot::B ? 2 : 0);
    const unsigned old=recording->mode.exchange(mode);
    if(mode!=old) recording->status.store(mode ? 1 : 0);
}
void beginSdSystemLog(const Configuration& configuration,const ActuatorRuntime& actuators,const RecentLogStore& recent,ILogger& logger) {
    if(recording) return;
    std::unique_ptr<RecordingService> candidate(new(std::nothrow) RecordingService);
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
}
void serviceSdSystemLog(const Configuration& configuration,const ActuatorRuntime& actuators,ILogger& logger) {
    configureSdSystemLog(configuration,actuators);
    if(!recording) return;
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
    return String(status<4 ? states[status] : "unknown")+" · queue="+String(static_cast<unsigned>(queued))+"/28 · dropped="+String(dropped);
}
}
