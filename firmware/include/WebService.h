#pragma once
#include "FirmwareWebRoutes.h"
#include "HardwareProvisioningWebRoutes.h"
#include "CommonSettingsWebRoutes.h"

#include <Arduino.h>
#include <WebServer.h>
#include <memory>
#include "ConfigurationBackup.h"
#include "IWebService.h"
#include "IWebRoutes.h"
#include "WebHost.h"
#include "IWebPagePresenter.h"
#include "NetworkWebRoutes.h"
#include "LogWebRoutes.h"
#include "IConfigurationService.h"
#include "IWiFiService.h"
#include "IMqttService.h"
#include "ITimeService.h"
#include "SensorManager.h"
#include "ActuatorRuntime.h"
#include "MeasurementSnapshotCache.h"
#include "LocaleFormatter.h"
#include "WebLocalization.h"
#include "RuntimeManager.h"
#include "ConfigurationRuntimeEffect.h"
#include "OTAService.h"
#include "Logger.h"
#include "IDiscoveryPublisher.h"
#include "ControllerRuntime.h"
#include "IRecentLogReader.h"
#include "I2CBusManager.h"
#include "BoardIdentityResolver.h"
#include "BoardProvisioningService.h"
#include "ModuleDescriptorProvisioningService.h"
#include "ValueRuntime.h"
#include "ModuleDeviceInventory.h"

namespace EnvNode {

class WebService : public IWebService, private IWebRoutes, private IWebPagePresenter, private INetworkSaveObserver, private IProvisioningAdministration, private IWebPageHeaderPresenter {
public:
    WebService(
        ILogger& logger,
        IConfigurationService& configurationService,
        ValueRuntime& valueRuntime,
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
        I2CBusManager& i2cBusManager,
        const BoardIdentityResolution& boardIdentityResolution,
        BoardProvisioningService& boardProvisioningService,
        ModuleDiscoveryService& moduleDiscoveryService,
        ModuleDescriptorProvisioningService& moduleDescriptorProvisioningService);

    void begin() override;
    void loop() override;

private:
    bool readProvisioningSnapshot(bool module,ModuleSlot slot,ProvisioningSnapshot& out) override { return module?moduleDescriptorProvisioningService_.snapshot(slot,out):boardProvisioningService_.snapshot(out); }
    BoardIdentity provisioningBoardIdentity() const override;
    const char* provisioningBoardBatch() const override { return boardIdentityResolution_.productionBatch; }
    const char* provisioningBoardDate() const override { return boardIdentityResolution_.productionDate; }
    size_t provisioningProfileCount() const override;
    bool provisioningProfileAt(size_t index, ProvisioningBoardProfile& profile) const override;
    bool provisioningProfile(BoardProfileId id, ProvisioningBoardProfile& profile) const override;
    bool provisioningProfile(BoardProfileId id, BoardRevision revision, ProvisioningBoardProfile& profile) const override;
    BoardProvisioningResult provisionBoardPortMap(const BoardPortMap&, bool) override;
    BoardProvisioningResult provisionBoardIdentity(const BoardIdentity& identity, bool confirmed) override;
    ModuleDescriptorProvisioningResult provisionModuleDescriptor(ModuleSlot slot,
        const uint8_t* payload, size_t size, bool confirmed) override;
    String renderHardwareOverview();
    String renderDeviceExtras();
    void registerRoutes(WebServer& server) override;
    Locale webLocale() const override;
    void networkSettingsSaved(bool success) override;
    void handleValues();
    void handleValueEdit();
    void handleValueSave();
    void handleValueSet();
    void handleValueDelete();
    void handleStatus();
    void handleSensors();
    void handleActuators();
    void handleControllers();
    void handleMeasurements();
    void handleDisplay();
    void handleDisplaySave();
    void handleDisplayOutputsSave();
    bool readDisplayOutputs(DisplayConfiguration& configuration);
    bool readDisplayEnumTranslations(DisplayPage& page, const IPropertyReader& properties);
    void handleSensorEdit();
    void handleSensorSave();
    void handleSensorApply();
    void handleActuatorEdit();
    void handleActuatorSave();
    void handleActuatorApply();
    void handleActuatorOn();
    void handleActuatorOff();
    void handleActuatorState(OnOffState state);
    void handleActuatorLevel();
    void handleControllerEdit();
    void handleControllerSave();
    bool readSelectorConfiguration(ControllerSlotConfiguration& slot);
    void handleControllerApply();
    void handleControllerStart();
    void handleControllerStop();
    void handleControllerRuntimeOperation(bool start);
    void handleMqtt();
    void handleUnits();
    void handleConfiguration();
    void handleConfigurationDownload();
    void handleConfigurationUploadData();
    void handleConfigurationPreview();
    void handleConfigurationRestore();
    String configurationUpload_;
    std::unique_ptr<Configuration> restoreCandidate_;
    bool configurationUploadFailed_ = false;
    bool configurationUploadComplete_ = false;
    bool restoreIncludesPasswords_ = false;
    uint32_t restorePreviewTime_ = 0;
    uint32_t restoreSequence_ = 0;
    String restoreToken_;
    void handleDiagnostics();
    void handleI2CScan();
    void handleMqttSave();
    void handleDiscoveryRepublish();
    void handleUnitsSave();
    void handleGeoPositionSave();
    void handlePressureReductionSave();
    void handleNotFound();

    void sendPage(const char* title, const char* activeRoute, const String& content,
        int status = 200, bool wideContent = false) override;
    void sendResult(const char* title, const char* activeRoute, const char* message, bool success);
    void sendConfigurationResult(
        const ConfigurationSaveResult& result,
        const char* successTitle,
        const char* failureTitle,
        const char* activeRoute,
        const char* failureMessage);
    const char* uiText(TextId id) const;
    String renderPageHeader(const char* title, const char* activeRoute,
        bool wideContent = false) const override;
    String currentLocalDateTime() const;
    String pendingRuntimeActionHtml() const;
    const char* pendingActionMessage() const;
    bool administrationAvailable() const;
    void renderDiagnostics();

    ILogger& logger_;
    IConfigurationService& configurationService_;
    ValueRuntime& valueRuntime_;
    IWiFiService& wifiService_;
    IMqttService& mqttService_;
    ITimeService& timeService_;
    LocaleFormatter& localeFormatter_;
    SensorManager& sensorManager_;
    ActuatorRuntime& actuatorRuntime_;
    ControllerRuntime& controllerRuntime_;
    MeasurementSnapshotCache& measurementSnapshotCache_;
    IDiscoveryPublisher& discoveryPublisher_;
    RuntimeManager& runtimeManager_;
    IDeviceRestart& deviceRestart_;
    OTAService& otaService_;
    I2CBusManager& i2cBusManager_;
    const BoardIdentityResolution& boardIdentityResolution_;
    BoardProvisioningService& boardProvisioningService_;
    ModuleDiscoveryService& moduleDiscoveryService_;
    ModuleDescriptorProvisioningService& moduleDescriptorProvisioningService_;
    WebHost webHost_;
    WebServer& server_;
    NetworkWebRoutes networkRoutes_;
    LogWebRoutes logRoutes_;
    std::unique_ptr<FirmwareWebRoutes> firmwareRoutes_;
    std::unique_ptr<HardwareProvisioningWebRoutes> provisioningRoutes_;
    std::unique_ptr<CommonSettingsWebRoutes> settingsRoutes_;
};

} // namespace EnvNode
