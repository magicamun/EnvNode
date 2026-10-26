// EnvNode: see repository LICENSE.
#pragma once
namespace EnvNode {
class IDeviceRestart {
public:
    virtual ~IDeviceRestart() = default;
    virtual void requestDeviceRestart() = 0;
    virtual bool deviceRestartPending() const = 0;
    virtual void performDeviceRestart() = 0;
};
}
