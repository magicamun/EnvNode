// EnvNode: see repository LICENSE.
#pragma once
#include "ActuatorId.h"
#include "IOnOffActuator.h"
#include "IDiagnosticRecorder.h"

namespace EnvNode {
// Called immediately after the hardware write (or a rejected command).
// The recorder captures device uptime here, before asynchronous UDP delivery.
inline void recordActuatorOutput(ActuatorId id, uint8_t gpio, uint8_t previous,
    uint8_t requested, uint8_t output, ActuatorOperationResult result, const char* action) {
    auto& recorder = diagnosticRecorder();
    const DiagnosticSource source(DiagnosticSourceKind::Actuator, id);
    if (!recorder.enabled(source)) return;
    DiagnosticEvent event("actuator_output");
    event.integer("gpio", gpio);
    event.integer("previous_percent", previous);
    event.integer("requested_percent", requested);
    event.integer("output_percent", output);
    event.text("operation", result == ActuatorOperationResult::Completed ? "completed"
        : result == ActuatorOperationResult::NotInitialized ? "not_initialized" : "invalid_hardware");
    event.text("action", action);
    (void)recorder.record(source, event);
}
}
