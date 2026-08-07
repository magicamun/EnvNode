#pragma once

#include "Logger.h"
#include "RuntimeAction.h"

namespace WeatherStation {

class RuntimeManager {
public:
    explicit RuntimeManager(ILogger& logger);

    void request(RuntimeAction action);
    RuntimeAction pendingAction() const;
    bool restartRequired() const;
    void service();
    void clearPending();

    // This is the firmware's single device-restart boundary. No other code may
    // call ESP.restart() directly.
    void performPendingRestart();

private:
    ILogger& logger_;
    RuntimeAction pendingAction_ = RuntimeAction::None;
};

} // namespace WeatherStation
