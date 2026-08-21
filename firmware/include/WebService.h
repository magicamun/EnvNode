#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "IWebService.h"
#include "IConfigurationService.h"
#include "IWiFiService.h"
#include "IMqttService.h"
#include "ITimeService.h"
#include "SensorManager.h"
#include "ActuatorRuntime.h"
#include "MeasurementSnapshotCache.h"
#include "LocaleFormatter.h"
#include "RuntimeManager.h"
#include "ConfigurationRuntimeEffect.h"
#include "OTAService.h"
#include "Logger.h"
#include "IDiscoveryPublisher.h"
#include "ControllerRuntime.h"
#include "IRecentLogReader.h"
#include "I2CBusManager.h"

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
        ActuatorRuntime& actuatorRuntime,
        ControllerRuntime& controllerRuntime,
        MeasurementSnapshotCache& measurementSnapshotCache,
        const IRecentLogReader& logReader,
        IDiscoveryPublisher& discoveryPublisher,
        RuntimeManager& runtimeManager,
        OTAService& otaService,
        I2CBusManager& i2cBusManager);

    void begin() override;
    void loop() override;

private:
    void handleStatus();
    void handleSensors();
    void handleActuators();
    void handleControllers();
    void handleMeasurements();
    void handleSensorEdit();
    void handleSensorSave();
    void handleSensorApply();
    void handleActuatorEdit();
    void handleActuatorSave();
    void handleActuatorApply();
    void handleActuatorOn();
    void handleActuatorOff();
    void handleActuatorState(OnOffState state);
    void handleControllerEdit();
    void handleControllerSave();
    void handleControllerApply();
    void handleControllerStart();
    void handleControllerStop();
    void handleControllerRuntimeOperation(bool start);
    void handleNetwork();
    void handleMqtt();
    void handleTime();
    void handleUnits();
    void handleDevice();
    void handleDiagnostics();
    void handleI2CScan();
    void handleLogs();
    void handleLogData();
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

    void sendPage(const char* title, const char* activeRoute, const String& content,
        int status = 200, bool wideContent = false);
    void sendResult(const char* title, const char* activeRoute, const char* message, bool success);
    void sendConfigurationResult(
        const ConfigurationSaveResult& result,
        const char* successTitle,
        const char* failureTitle,
        const char* activeRoute,
        const char* failureMessage);
    String renderPage(const char* title, const char* activeRoute, const String& content,
        bool wideContent = false) const;
    String currentLocalDateTime() const;
    String pendingRuntimeActionHtml() const;
    String otaStatusHtml() const;
    const char* pendingActionMessage() const;
    void performExplicitRestart();
    bool administrationAvailable() const;
    void renderDiagnostics(bool scanI2CBuses);

    ILogger& logger_;
    IConfigurationService& configurationService_;
    IWiFiService& wifiService_;
    IMqttService& mqttService_;
    ITimeService& timeService_;
    LocaleFormatter& localeFormatter_;
    SensorManager& sensorManager_;
    ActuatorRuntime& actuatorRuntime_;
    ControllerRuntime& controllerRuntime_;
    MeasurementSnapshotCache& measurementSnapshotCache_;
    const IRecentLogReader& logReader_;
    IDiscoveryPublisher& discoveryPublisher_;
    RuntimeManager& runtimeManager_;
    OTAService& otaService_;
    I2CBusManager& i2cBusManager_;
    WebServer server_{80};
    bool firmwareUploadRequestAccepted_ = false;
    String firmwareUploadRequestError_;
};

} // namespace EnvNode
