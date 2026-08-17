#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "IWebService.h"
#include "IConfigurationService.h"
#include "IWiFiService.h"
#include "IMqttService.h"
#include "ITimeService.h"
#include "SensorManager.h"
#include "MeasurementSnapshotCache.h"
#include "LocaleFormatter.h"
#include "RuntimeManager.h"
#include "ConfigurationRuntimeEffect.h"
#include "OTAService.h"
#include "Logger.h"
#include "IDiscoveryPublisher.h"
#include "IOnOffActuator.h"

namespace EnvNode {

class WebService : public IWebService {
public:
    WebService(
        ILogger& logger,
        IConfigurationService& configurationService,
        IWiFiService& wifiService,
        IMqttService& mqttService,
        ITimeService& timeService,
        LocaleFormatter& localeFormatter,
        SensorManager& sensorManager,
        MeasurementSnapshotCache& measurementSnapshotCache,
        IDiscoveryPublisher& discoveryPublisher,
        RuntimeManager& runtimeManager,
        OTAService& otaService,
        IOnOffActuator& temporaryLedActuator);

    void begin() override;
    void loop() override;

private:
    void handleStatus();
    void handleSensors();
    void handleMeasurements();
    void handleActuator();
    void handleActuatorOn();
    void handleActuatorOff();
    void handleSensorEdit();
    void handleSensorSave();
    void handleSensorApply();
    void handleNetwork();
    void handleMqtt();
    void handleTime();
    void handleUnits();
    void handleDevice();
    void handleDiagnostics();
    void handleFirmware();
    void handleFirmwareUpload();
    void handleFirmwareUploadData();
    void handleStyle();
    void handleNetworkSave();
    void handleMqttSave();
    void handleDiscoveryRepublish();
    void handleTimeSave();
    void handleUnitsSave();
    void handleDeviceSave();
    void handleRestart();
    void handleFactoryReset();
    void handleNotFound();

    void sendPage(const char* title, const char* activeRoute, const String& content, int status = 200);
    void sendResult(const char* title, const char* activeRoute, const char* message, bool success);
    void sendConfigurationResult(
        const ConfigurationSaveResult& result,
        const char* successTitle,
        const char* failureTitle,
        const char* activeRoute,
        const char* failureMessage);
    String renderPage(const char* title, const char* activeRoute, const String& content) const;
    String navigationHtml(const char* activeRoute) const;
    String currentLocalDateTime() const;
    String pendingRuntimeActionHtml() const;
    String otaStatusHtml() const;
    const char* pendingActionMessage() const;
    void performExplicitRestart();
    bool administrationAvailable() const;
    void setTemporaryLedState(OnOffState state);

    ILogger& logger_;
    IConfigurationService& configurationService_;
    IWiFiService& wifiService_;
    IMqttService& mqttService_;
    ITimeService& timeService_;
    LocaleFormatter& localeFormatter_;
    SensorManager& sensorManager_;
    MeasurementSnapshotCache& measurementSnapshotCache_;
    IDiscoveryPublisher& discoveryPublisher_;
    RuntimeManager& runtimeManager_;
    OTAService& otaService_;
    IOnOffActuator& temporaryLedActuator_;
    WebServer server_{80};
    bool firmwareUploadRequestAccepted_ = false;
    String firmwareUploadRequestError_;
};

} // namespace EnvNode
