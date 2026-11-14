// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
#include "Sct013Sensor.h"
#include "IDiagnosticRecorder.h"
namespace EnvNode {
void Sct013Sensor::begin() {
    window_={}; active_=false;
    state_=settings_.valid() && input_.begin(gpio_,settings_.filterHz) ? SensorState::Initializing : SensorState::Failed;
}
SensorOperationResult Sct013Sensor::service(IMeasurementSink&) {
    SctWindow w;
    if (state_==SensorState::Failed || !input_.latest(w) || w.revision==window_.revision)
        return SensorOperationResult::NoData;
    window_=w;
    state_=w.valid()?SensorState::Ready:SensorState::Degraded;
    const double scale=settings_.ratedA*settings_.cal/(0.5*settings_.ratedMv);
    const float filtered=w.filteredMv*scale;
    if (w.valid()) {
        if (!settings_.onMa || filtered > settings_.onMa/1000.0) active_=true;
        else if (filtered < settings_.offMa/1000.0) active_=false;
        outputA_=active_ ? filtered : 0;
    }
    auto& recorder=diagnosticRecorder();
    DiagnosticSource source(DiagnosticSourceKind::Sensor,id_);
    if (recorder.enabled(source)) {
#ifdef ENVNODE_SCT_DMA
        if(w.pausedForUpload) {
            DiagnosticEvent paused("sct013_ota_pause");
            paused.text("state","paused");
            recorder.record(source,paused);
            return SensorOperationResult::NoData;
        }
        if(w.recoveryAttempts) {
            DiagnosticEvent recovery("sct013_dma_recovery");
            recovery.integer("attempts",w.recoveryAttempts);
            recovery.integer("successes",w.recoverySuccesses);
            recovery.integer("last_code",w.lastRecoveryError);
            recovery.text("state",w.timingErrors?"invalid":"sampling");
            recorder.record(source,recovery);
        }
        if (w.timingErrors) {
            DiagnosticEvent error("sct013_dma_error");
            error.integer("read_errors",w.readErrors);
            error.integer("last_read_code",w.lastReadError);
            error.integer("channel_errors",w.channelErrors);
            error.integer("last_bad_channel",w.unexpectedChannel);
            error.integer("late_polls",w.pollErrors);
            error.integer("length_errors",w.lengthErrors);
            recorder.record(source,error);
            return SensorOperationResult::NoData;
        }
#endif
        DiagnosticEvent event("sct013_window");
        event.number("bias_mV",w.bias); event.number("raw_A",w.rawMv*scale);
        event.number("filtered_A",filtered); event.integer("timing_errors",w.timingErrors);
        event.text("fault",w.timingErrors?"timing":w.bias<1400 || w.bias>1900?"bias":
            w.minimum<150 || w.maximum>2450?"range":"none");
        event.text("state",!w.valid()?"invalid":active_?"active":"zero");
        recorder.record(source,event);
#ifdef ENVNODE_SCT_DMA
        DiagnosticEvent timing("sct013_dma");
        timing.text("acquisition","adc_dma");
        timing.integer("revision",w.revision);
        timing.integer("rate_hz",2000);
        timing.integer("dma_errors",w.timingErrors);
#else
        DiagnosticEvent timing("sct013_timing");
        timing.integer("revision",w.revision);
        timing.integer("short_intervals",w.timing.shortIntervals);
        timing.integer("long_intervals",w.timing.longIntervals);
        timing.integer("max_gap_us",w.timing.maxGapUs);
        timing.integer("span_us",w.timing.spanUs);
        timing.integer("timing_errors",w.timingErrors);
#endif
        recorder.record(source,timing);
    }
    return SensorOperationResult::NoData;
}
SensorOperationResult Sct013Sensor::sample(IMeasurementSink& sink) {
    service(sink);
    Measurement m;
    m.type=MeasurementType::ElectricCurrent;
    m.valid=window_.valid() && uint32_t(clock_.nowMs()-window_.endMs)<=1500;
    if (m.valid) m.value=MeasurementValue::floatingPoint(outputA_);
    m.quality=m.valid?MeasurementQuality::Good:MeasurementQuality::Degraded;
    sink.emit(m);
    return SensorOperationResult::Completed;
}
}
