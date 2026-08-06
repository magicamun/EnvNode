#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "IWebService.h"
#include "IConfigurationService.h"
#include "IWiFiService.h"
#include "Logger.h"

namespace WeatherStation {

class WebService : public IWebService {
public:
    WebService(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService);

    void begin() override;
    void loop() override;

private:
    void handleRoot();
    void handleSave();
    void handleReset();
    void handleNotFound();
    void scheduleRestart();
    String configurationPage() const;
    String responsePage(const char* title, const char* message) const;
    bool isProvisioningEnabled() const;

    ILogger& logger_;
    IConfigurationService& configurationService_;
    IWiFiService& wifiService_;
    WebServer server_{80};
    unsigned long restartAtMs_ = 0;
};

} // namespace WeatherStation
