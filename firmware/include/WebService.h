#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "IWebService.h"
#include "IConfigurationService.h"
#include "IWiFiService.h"
#include "IMqttService.h"
#include "ITimeService.h"
#include "SensorManager.h"
#include "Logger.h"

namespace WeatherStation {

class WebService : public IWebService {
public:
    WebService(
        ILogger& logger,
        IConfigurationService& configurationService,
        IWiFiService& wifiService,
        IMqttService& mqttService,
        ITimeService& timeService,
        SensorManager& sensorManager);

    void begin() override;
    void loop() override;

private:
    void handleStatus();
    void handleSensors();
    void handleNetwork();
    void handleMqtt();
    void handleTime();
    void handleUnits();
    void handleDevice();
    void handleDiagnostics();
    void handleFirmware();
    void handleStyle();
    void handleNetworkSave();
    void handleMqttSave();
    void handleTimeSave();
    void handleUnitsSave();
    void handleDeviceSave();
    void handleRestart();
    void handleFactoryReset();
    void handleNotFound();

    void sendPage(const char* title, const char* activeRoute, const String& content, int status = 200);
    void sendResult(const char* title, const char* activeRoute, const char* message, bool success);
    String renderPage(const char* title, const char* activeRoute, const String& content) const;
    String navigationHtml(const char* activeRoute) const;
    void scheduleRestart();
    bool administrationAvailable() const;

    ILogger& logger_;
    IConfigurationService& configurationService_;
    IWiFiService& wifiService_;
    IMqttService& mqttService_;
    ITimeService& timeService_;
    SensorManager& sensorManager_;
    WebServer server_{80};
    unsigned long restartAtMs_ = 0;
};

} // namespace WeatherStation
