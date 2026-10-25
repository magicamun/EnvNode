#pragma once

#include "Logger.h"
#include "IDeviceRestart.h"
#include "DeviceRestartExecutor.h"
#include "RuntimeAction.h"
#include "ISensorRuntime.h"
#include "ActuatorRuntime.h"
#include "ControllerRuntime.h"

namespace EnvNode {

class RuntimeManager : public IDeviceRestart {
public:
    explicit RuntimeManager(
        ILogger& logger,
        ISensorRuntime* sensorRuntime = nullptr,
        ActuatorRuntime* actuatorRuntime = nullptr,
        ControllerRuntime* controllerRuntime = nullptr,
        IDeviceRestartDriver* restartDriver = nullptr);

    void request(RuntimeAction action);
    RuntimeAction pendingAction() const;
    bool restartRequired() const;
    void service();
    void clearPending();
    bool applyPendingSensorChanges();
    bool applyPendingActuatorChanges(const ActuatorSlotConfiguration* slots);
    bool applyPendingControllerChanges(const ControllerSlotConfiguration* slots);

    // Pending-action gate for explicit device restart. The shared executor owns
    // the only ESP.restart() call.
    void performPendingRestart();
    void requestDeviceRestart() override;
    bool deviceRestartPending() const override;
    void performDeviceRestart() override;

private:
    ILogger& logger_;
    ISensorRuntime* sensorRuntime_;
    ActuatorRuntime* actuatorRuntime_;
    ControllerRuntime* controllerRuntime_;
    DeviceRestartExecutor restartExecutor_;
    RuntimeAction pendingAction_ = RuntimeAction::None;
};

} // namespace EnvNode
