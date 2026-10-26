// EnvNode: see repository LICENSE.
#pragma once
#include "IFirmwareApplication.h"
namespace EnvNode {
class IWiFiService;
class ITimeService;
class IWebService;
class FirmwareRuntime {
public:
    FirmwareRuntime(IWiFiService& wifi, ITimeService& time, IWebService& web,
        IFirmwareApplication& application);
    void begin();
    void loop();
private:
    IWiFiService& wifi_;
    ITimeService& time_;
    IWebService& web_;
    IFirmwareApplication& application_;
    bool started_ = false;
};
}
