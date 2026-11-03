// EnvNode: see repository LICENSE.
#include "SystemDiagnostics.h"
#include <Arduino.h>
namespace EnvNode {
namespace {bool wasEnabled=false;uint32_t lastProduced=0;}
void serviceSystemDiagnostics() {
    auto& recorder=diagnosticRecorder();
    const DiagnosticSource source(DiagnosticSourceKind::System,0);
    const bool enabled=recorder.enabled(source);const uint32_t now=millis();
    if(enabled && (!wasEnabled || now-lastProduced>=60000)) {
        DiagnosticEvent event("runtime_health");event.integer("free_heap",ESP.getFreeHeap());event.integer("largest_heap",ESP.getMaxAllocHeap());
        recorder.record(source,event);lastProduced=now;
    }
    wasEnabled=enabled;
}
DiagnosticRecordResult recordDiagnosticTest() {
    DiagnosticEvent event("recorder_test");event.integer("free_heap",ESP.getFreeHeap());event.integer("largest_heap",ESP.getMaxAllocHeap());
    event.text("origin","web");return diagnosticRecorder().record({DiagnosticSourceKind::System,0},event);
}
}
