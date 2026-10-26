// EnvNode: see repository LICENSE.
#pragma once
#include "IDeviceRestart.h"
#include "DeviceRestartExecutor.h"
namespace EnvNode {
class TestBoardRestart : public IDeviceRestart {
public:
    explicit TestBoardRestart(ILogger& logger, IDeviceRestartDriver* driver = nullptr)
        : executor_(logger, driver) {}
    void requestDeviceRestart() override { pending_ = true; }
    bool deviceRestartPending() const override { return pending_; }
    void performDeviceRestart() override { executor_.execute(); }
private:
    DeviceRestartExecutor executor_;
    bool pending_ = false;
};
}
