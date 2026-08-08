#pragma once

#include "Logger.h"
#include "RuntimeAction.h"
#include "ISensorRuntime.h"

namespace WeatherStation {

class RuntimeManager {
public:
    explicit RuntimeManager(ILogger& logger, ISensorRuntime* sensorRuntime = nullptr);

    void request(RuntimeAction action);
    RuntimeAction pendingAction() const;
    bool restartRequired() const;
    void service();
    void clearPending();
    bool applyPendingSensorChanges();

    // This is the firmware's single device-restart boundary. No other code may
    // call ESP.restart() directly.
    void performPendingRestart();

private:
    ILogger& logger_;
    ISensorRuntime* sensorRuntime_;
    RuntimeAction pendingAction_ = RuntimeAction::None;
};

} // namespace WeatherStation
