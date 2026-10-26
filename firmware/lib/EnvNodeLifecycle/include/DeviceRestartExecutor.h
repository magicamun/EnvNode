// EnvNode: see repository LICENSE.
#pragma once
#include "Logger.h"
namespace EnvNode {
class IDeviceRestartDriver {
public:
    virtual ~IDeviceRestartDriver() = default;
    virtual void restart() = 0;
};
// Executes an already authorized restart; pending-action policy belongs to the application.
class DeviceRestartExecutor {
public:
    explicit DeviceRestartExecutor(ILogger& logger, IDeviceRestartDriver* driver = nullptr);
    void execute();
private:
    ILogger& logger_;
    IDeviceRestartDriver& driver_;
};
}
