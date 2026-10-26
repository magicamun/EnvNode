// EnvNode: see repository LICENSE.
#include "FirmwareRuntime.h"
#include "IWiFiService.h"
#include "ITimeService.h"
#include "IWebService.h"
namespace EnvNode {
FirmwareRuntime::FirmwareRuntime(IWiFiService& wifi, ITimeService& time,
    IWebService& web, IFirmwareApplication& application)
    : wifi_(wifi), time_(time), web_(web), application_(application) {}
void FirmwareRuntime::begin() {
    if (started_) return;
    wifi_.begin();
    time_.begin();
    application_.initializeDevices();
    web_.begin();
    application_.beginIntegrations();
    started_ = true;
}
void FirmwareRuntime::loop() {
    if (!started_) return;
    wifi_.loop();
    time_.loop();
    application_.serviceMeasurements();
    web_.loop();
    application_.serviceIntegrations();
}
}
