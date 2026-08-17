#pragma once

#include "Logger.h"
#include "RuntimeAction.h"
#include "ISensorRuntime.h"
#include "ActuatorRuntime.h"

namespace EnvNode {

class RuntimeManager {
public:
    explicit RuntimeManager(
        ILogger& logger,
        ISensorRuntime* sensorRuntime = nullptr,
        ActuatorRuntime* actuatorRuntime = nullptr);

    void request(RuntimeAction action);
    RuntimeAction pendingAction() const;
    bool restartRequired() const;
    void service();
    void clearPending();
    bool applyPendingSensorChanges();
    bool applyPendingActuatorChanges(const ActuatorSlotConfiguration* slots);

    // This is the firmware's single device-restart boundary. No other code may
    // call ESP.restart() directly.
    void performPendingRestart();

private:
    ILogger& logger_;
    ISensorRuntime* sensorRuntime_;
    ActuatorRuntime* actuatorRuntime_;
    RuntimeAction pendingAction_ = RuntimeAction::None;
};

} // namespace EnvNode
