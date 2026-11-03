// EnvNode: see repository LICENSE.
#include "SystemDiagnostics.h"
#include <Arduino.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif
namespace EnvNode {
namespace {
bool wasEnabled=false;uint32_t lastProduced=0;
uint32_t freeDataHeap(){
#if defined(ARDUINO_ARCH_ESP32)
    return heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
#else
    return ESP.getFreeHeap();
#endif
}
uint32_t largestDataHeap(){
#if defined(ARDUINO_ARCH_ESP32)
    return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
#else
    return ESP.getMaxAllocHeap();
#endif
}
}
void serviceSystemDiagnostics() {
    auto& recorder=diagnosticRecorder();
    const DiagnosticSource source(DiagnosticSourceKind::System,0);
    const bool enabled=recorder.enabled(source);const uint32_t now=millis();
    if(enabled && (!wasEnabled || now-lastProduced>=60000)) {
        DiagnosticEvent event("runtime_health");event.integer("free_heap_8bit",freeDataHeap());event.integer("largest_heap_8bit",largestDataHeap());
        recorder.record(source,event);lastProduced=now;
    }
    wasEnabled=enabled;
}
DiagnosticRecordResult recordDiagnosticTest() {
    DiagnosticEvent event("recorder_test");event.integer("free_heap_8bit",freeDataHeap());event.integer("largest_heap_8bit",largestDataHeap());
    event.text("origin","web");return diagnosticRecorder().record({DiagnosticSourceKind::System,0},event);
}
}
