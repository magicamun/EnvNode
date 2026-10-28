#include "BoardIdentityProfiles.h"
#include "TextUpload.h"
#include "WebResult.h"
#include "InstanceUuidWebText.h"
#include "HttpRedirect.h"
#include "ControllerTargetIdentity.h"
#include "WebService.h"
#include "WebLocalization.h"
#include "WebPresentationText.h"
#include "HtmlEscaping.h"
#include "PropertyResolver.h"
#include "PropertyPresentationReader.h"
#include "SystemPropertyReader.h"
#include "ValuePropertyReader.h"
#include "PropertyPreviewWebView.h"
#include "PageEditorScript.h"
#include "WebNavigation.h"
#include "WebPageFrame.h"
#include "ValueWebView.h"
#include "MqttTopic.h"
#include "BoardProfile.h"
#include "PressureReductionWebView.h"
#include "GeoPositionWebView.h"
#include "LocalizedDecimal.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include "FirmwareVersion.h"
#include "FirmwareBuildInfo.h"
#include "UnitConverter.h"
#include "SensorImplementationRegistry.h"
#include "SensorSlotConfiguration.h"
#include "ActuatorImplementationRegistry.h"
#include "ControllerImplementationRegistry.h"
#include "ControllerWebSupport.h"
#include "SelectorWebView.h"
#include "ElapsedTimeFormatter.h"

namespace EnvNode {
namespace {

String moduleActuatorToken(const ModuleActuatorReference& reference) {
    String token;
    const auto append = [&token](const uint8_t* bytes, size_t length) {
        for (size_t index = 0; index < length; ++index) {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", bytes[index]);
            token += hex;
        }
    };
    append(reference.moduleInstanceFingerprint, sizeof(reference.moduleInstanceFingerprint));
    append(reference.deviceIdHash, sizeof(reference.deviceIdHash));
    return token;
}

bool actuatorInfoById(const ActuatorRuntime& runtime, ActuatorId id, ActuatorRuntimeInfo& info) {
    for (size_t index = 0; index < runtime.runtimeCount(); ++index) {
        if (runtime.runtimeInfo(index, info) && info.id == id) return true;
    }
    return false;
}


String badge(const char* text, const char* style) {
    return "<span class='badge " + String(style) + "'>" + text + "</span>";
}

String descriptorText(const DescriptorTextView& view) {
    String result;
    result.reserve(view.size);
    for (size_t index = 0; index < view.size; ++index) result += view.data[index];
    return result;
}

String moduleActuatorLabel(const ModuleDiscoveryService& discovery,
    const ModuleActuatorReference& reference, Locale locale) {
    for (size_t index = 0; index < ModuleDiscoveryService::SlotCount; ++index) {
        const auto* module = discovery.result(static_cast<ModuleSlot>(index));
        if (module == nullptr || !module->identified() || !module->descriptorHasInstanceId
            || memcmp(reference.moduleInstanceFingerprint, module->descriptorInstanceId,
                sizeof(reference.moduleInstanceFingerprint)) != 0) continue;
        return descriptorText(module->descriptorName.empty()
            ? module->descriptorTypeId : module->descriptorName)
            + (String(" · ") + webText(locale, TextId::Slot) + " ") + moduleSlotName(module->slot);
    }
    return webText(locale, TextId::ModuleDescriptor);
}


const char* sensorStateName(SensorState state, Locale locale) {
    switch (state) {
        case SensorState::Initializing: return webText(locale, TextId::Initializing);
        case SensorState::Ready: return webText(locale, TextId::Ready);
        case SensorState::Degraded: return webText(locale, TextId::Degraded);
        case SensorState::Failed: return webText(locale, TextId::Failed);
        default: return webText(locale, TextId::Unknown);
    }
}

const char* onOffStateName(OnOffState state, Locale locale) {
    return state == OnOffState::On ? webText(locale, TextId::On) : webText(locale, TextId::Off);
}

const char* actuatorOperationFailure(ActuatorOperationResult result, Locale locale) {
    switch (result) {
        case ActuatorOperationResult::InvalidHardwareResource:
            return webText(locale, TextId::TheActuatorHardwareResourceIsInvalid);
        case ActuatorOperationResult::NotInitialized:
            return webText(locale, TextId::TheActuatorIsNotInitialized);
        case ActuatorOperationResult::Completed:
            return webText(locale, TextId::TheActuatorOperationCompleted);
        default:
            return webText(locale, TextId::TheActuatorOperationFailed);
    }
}

const char* blinkPhaseName(BlinkPhase phase, Locale locale) {
    switch (phase) {
        case BlinkPhase::Stopped: return webText(locale, TextId::Stopped);
        case BlinkPhase::WaitingForTarget: return webText(locale, TextId::WaitingForTarget);
        case BlinkPhase::On: return webText(locale, TextId::On);
        case BlinkPhase::Off: return webText(locale, TextId::Off);
        default: return webText(locale, TextId::Unknown);
    }
}

const char* thresholdDecisionName(ThresholdDecision decision, Locale locale) {
    switch (decision) {
        case ThresholdDecision::On: return webText(locale, TextId::On);
        case ThresholdDecision::Off: return webText(locale, TextId::Off);
        case ThresholdDecision::Unknown:
        default: return webText(locale, TextId::Unknown);
    }
}

const char* controllerOperationName(ControllerOperationResult result, Locale locale) {
    switch (result) {
        case ControllerOperationResult::Completed: return webText(locale, TextId::Completed);
        case ControllerOperationResult::NoAction: return webText(locale, TextId::NoActionRequired);
        case ControllerOperationResult::TargetUnavailable: return webText(locale, TextId::TargetUnavailable);
        case ControllerOperationResult::ActuatorOperationFailed: return webText(locale, TextId::ActuatorOperationFailed);
        case ControllerOperationResult::InvalidConfiguration: return webText(locale, TextId::InvalidConfiguration);
        case ControllerOperationResult::NotRunning: return webText(locale, TextId::NotRunning);
        case ControllerOperationResult::ControllerNotFound: return webText(locale, TextId::ControllerNotFound);
        default: return webText(locale, TextId::Unknown);
    }
}

bool parseControllerDuration(const String& text, uint32_t& duration) {
    if (text.isEmpty()) return false;
    char* end = nullptr;
    const unsigned long long parsed = strtoull(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0' || parsed == 0 || parsed > INT32_MAX) {
        return false;
    }
    duration = static_cast<uint32_t>(parsed);
    return true;
}

String lastMeasurementDisplay(
    const SensorRuntimeStatus& status,
    LocaleFormatter& localeFormatter, Locale locale) {
    if (!status.hasMeasurementActivity) return "—";

    String result;
    if (status.hasLastMeasurement) {
        tm localTime;
        const time_t timestamp = status.lastMeasurementEpoch;
        result = localtime_r(&timestamp, &localTime) != nullptr
            ? localeFormatter.formatDateTime(localTime)
            : String("—");
    } else {
        result = webText(locale, TextId::PreSyncActivity);
    }

    const uint32_t ageMs = millis() - status.lastMeasurementMonotonicMs;
    result += "<br><span class='help'>";
    result += formatElapsedDuration(ageMs);
    result += (String("") + String(" ") + webText(locale, TextId::Ago) + "</span>");
    return result;
}


String unitSelect(const char* name, MeasurementType type, PresentationUnit current, Locale locale) {
    const MeasurementTypeMetadata& metadata = measurementTypeMetadata(type);
    String html;
    html.reserve(64 + metadata.supportedPresentationUnitCount * 100);
    html = "<select name='";
    html += name;
    html += "'>";
    for (uint8_t index = 0; index < metadata.supportedPresentationUnitCount; ++index) {
        const PresentationUnit unit = metadata.supportedPresentationUnits[index];
        html += "<option value='" + String(UnitConverter::stableKey(unit)) + "'" + (unit == current ? " selected" : "") + ">";
        html += webUnitName(locale, unit);
        html += " (" + String(UnitConverter::symbol(unit)) + ")</option>";
    }
    html += "</select>";
    return html;
}

bool parseUnit(const String& value, MeasurementType type, PresentationUnit& unit) {
    return UnitConverter::parseStableKey(value.c_str(), unit) && supportsPresentationUnit(type, unit);
}

String measurementTypes(const SensorRuntimeInfo& info, Locale locale) {
    String result;
    result.reserve(180);
    const auto appendType = [&result, locale](bool supported, MeasurementType type) {
        if (supported) result += String(webMeasurementName(locale, type)) + ", ";
    };
    appendType(info.supportsTemperature, MeasurementType::Temperature);
    appendType(info.supportsRelativeHumidity, MeasurementType::RelativeHumidity);
    appendType(info.supportsAtmosphericPressure, MeasurementType::AtmosphericPressure);
    appendType(info.supportsSeaLevelPressure, MeasurementType::SeaLevelPressure);
    appendType(info.supportsSolarIrradiance, MeasurementType::SolarIrradiance);
    appendType(info.supportsSolarCellTemperature, MeasurementType::SolarCellTemperature);
    appendType(info.supportsRainDetectorLevel, MeasurementType::RainDetectorLevel);
    appendType(info.supportsRainDetectorWet, MeasurementType::RainDetectorWet);
    appendType(info.supportsRainGaugeTip, MeasurementType::RainGaugeTip);
    appendType(info.supportsRainfallIncrement, MeasurementType::RainfallIncrement);
    appendType(info.supportsHydrostaticPressure, MeasurementType::HydrostaticPressure);
    appendType(info.supportsWaterLevel, MeasurementType::WaterLevel);
    if (result.endsWith(", ")) result.remove(result.length() - 2);
    return result;
}

String hardwareAssignment(const SensorRuntimeInfo& info, Locale locale) {
    if (info.hardware.kind == HardwareResourceKind::GPIO) {
        return "GPIO" + String(info.hardware.gpio.number);
    }
    if (info.hardware.kind == HardwareResourceKind::I2C) {
        return String(i2cBusName(info.hardware.i2c.bus)) + " / 0x"
            + String(info.hardware.i2c.address, HEX);
    }
    return webText(locale, TextId::None);
}

String configuredHardwareAssignment(const HardwareResourceAssignment& hardware, Locale locale) {
    if (hardware.kind == HardwareResourceKind::GPIO) {
        return "GPIO" + String(hardware.gpio.number);
    }
    if (hardware.kind == HardwareResourceKind::I2C) {
        return String(i2cBusName(hardware.i2c.bus)) + " / 0x"
            + String(hardware.i2c.address, HEX);
    }
    return webText(locale, TextId::None);
}

bool sameHardwareAssignment(
    const HardwareResourceAssignment& first,
    const HardwareResourceAssignment& second) {
    if (first.kind != second.kind) return false;
    if (first.kind == HardwareResourceKind::GPIO) {
        return first.gpio.number == second.gpio.number;
    }
    if (first.kind == HardwareResourceKind::I2C) {
        return first.i2c.bus == second.i2c.bus && first.i2c.address == second.i2c.address;
    }
    return true;
}

bool gpioAssignedToOtherEnabledSlot(
    const Configuration& configuration,
    SensorId editedSensorSlotId,
    ActuatorId editedActuatorSlotId,
    GpioResource gpio) {
    const HardwareResourceAssignment candidate =
        HardwareResourceAssignment::gpioResource(gpio);
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        const SensorSlotConfiguration& other = configuration.sensorSlots[index];
        if (other.slotId == editedSensorSlotId
            || !other.enabled
            || other.implementation == SensorImplementation::None) {
            continue;
        }
        if (exclusiveHardwareResourceConflict(other.hardware, candidate)) return true;
    }
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        const ActuatorSlotConfiguration& other = configuration.actuatorSlots[index];
        if (other.slotId == editedActuatorSlotId
            || !other.enabled
            || other.implementation == ActuatorImplementation::None) {
            continue;
        }
        if (exclusiveHardwareResourceConflict(other.hardware, candidate)) return true;
    }
    return false;
}

String implementationMeasurements(const SensorImplementationMetadata* metadata, Locale locale) {
    if (metadata == nullptr || metadata->measurementTypeCount == 0) return webText(locale, TextId::None);
    String result;
    for (size_t index = 0; index < metadata->measurementTypeCount; ++index) {
        result += webMeasurementName(locale, metadata->measurementTypes[index]);
        if (index + 1 < metadata->measurementTypeCount) result += ", ";
    }
    return result;
}

String availableValue(const String& value) {
    return value.isEmpty() || value == "0.0.0.0" ? String("—") : escapeHtml(value);
}

String effectiveConnectionMode(bool setupAccessPoint, Locale locale) {
    if (setupAccessPoint) return String("—");
    esp_netif_t* stationInterface = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_dhcp_status_t dhcpStatus = ESP_NETIF_DHCP_INIT;
    if (stationInterface == nullptr
        || esp_netif_dhcpc_get_status(stationInterface, &dhcpStatus) != ESP_OK) {
        return String("—");
    }
    return dhcpStatus == ESP_NETIF_DHCP_STARTED ? String("DHCP")
        : dhcpStatus == ESP_NETIF_DHCP_STOPPED ? String(webText(locale, TextId::StaticIP))
        : String("—");
}

bool runtimeSupportsMeasurement(const SensorRuntimeInfo& info, MeasurementType type) {
    switch (type) {
        case MeasurementType::Temperature: return info.supportsTemperature;
        case MeasurementType::RelativeHumidity: return info.supportsRelativeHumidity;
        case MeasurementType::SeaLevelPressure: return info.supportsSeaLevelPressure;
        case MeasurementType::AtmosphericPressure: return info.supportsAtmosphericPressure;
        case MeasurementType::SolarIrradiance: return info.supportsSolarIrradiance;
        case MeasurementType::SolarCellTemperature: return info.supportsSolarCellTemperature;
        case MeasurementType::RainDetectorLevel: return info.supportsRainDetectorLevel;
        case MeasurementType::RainDetectorWet: return info.supportsRainDetectorWet;
        case MeasurementType::RainGaugeTip: return info.supportsRainGaugeTip;
        case MeasurementType::RainfallIncrement: return info.supportsRainfallIncrement;
        case MeasurementType::HydrostaticPressure: return info.supportsHydrostaticPressure;
        case MeasurementType::WaterLevel: return info.supportsWaterLevel;
        default: return false;
    }
}

const char* measurementQualityName(MeasurementQuality quality, Locale locale) {
    return webQualityName(locale, quality);
}

String presentedMeasurementValue(
    const Measurement& measurement,
    const Configuration& configuration,
    LocaleFormatter& localeFormatter, Locale locale) {
    if (!measurement.valid) return webText(locale, TextId::Invalid);
    const MeasurementTypeMetadata& metadata = measurementTypeMetadata(measurement.type);
    switch (measurement.value.kind()) {
        case ValueKind::FloatingPoint: {
            float canonicalValue = 0.0F;
            if (!measurement.value.tryGetFloatingPoint(canonicalValue)) return webText(locale, TextId::Invalid);
            PresentationUnit unit = UnitConverter::resolvePresentationUnit(measurement.type, configuration.presentationUnitFor(measurement.type));
            float presentedValue = 0.0F;
            if (!UnitConverter::convert(measurement.type, canonicalValue, unit, presentedValue)) {
                return webText(locale, TextId::Invalid);
            }
            String result = localeFormatter.formatNumber(
                presentedValue, metadata.recommendedDisplayPrecision);
            const char* symbol = UnitConverter::symbol(unit);
            if (symbol != nullptr && symbol[0] != '\0') result += " " + String(symbol);
            return result;
        }
        case ValueKind::Boolean: {
            bool value = false;
            return measurement.value.tryGetBoolean(value)
                ? String(value ? webText(locale, TextId::True) : webText(locale, TextId::False)) : String(webText(locale, TextId::Invalid));
        }
        case ValueKind::UnsignedInteger: {
            uint32_t value = 0;
            return measurement.value.tryGetUnsignedInteger(value)
                ? localeFormatter.formatNumber(value, 0) : String(webText(locale, TextId::Invalid));
        }
        case ValueKind::None:
            return metadata.semantics == MeasurementSemantics::Event ? String(webText(locale, TextId::Event)) : String("—");
        default:
            return webText(locale, TextId::Invalid);
    }
}

String measurementTimeDisplay(
    const MeasurementSnapshot& snapshot,
    LocaleFormatter& localeFormatter, Locale locale) {
    tm localTime;
    const time_t timestamp = snapshot.measurement.timestamp;
    if (localtime_r(&timestamp, &localTime) == nullptr) return "—";
    String result = localeFormatter.formatDateTime(localTime);
    result += "<br><span class='help'>";
    result += formatElapsedDuration(millis() - snapshot.acceptedMonotonicMs);
    result += (String("") + String(" ") + webText(locale, TextId::Ago) + "</span>");
    return result;
}

} // namespace

WebService::WebService(ILogger& logger, IConfigurationService& configurationService, ValueRuntime& valueRuntime, IWiFiService& wifiService,
    IMqttService& mqttService, ITimeService& timeService, LocaleFormatter& localeFormatter,
    SensorManager& sensorManager, ActuatorRuntime& actuatorRuntime,
    ControllerRuntime& controllerRuntime,
    MeasurementSnapshotCache& measurementSnapshotCache,
    const IRecentLogReader& logReader,
    IDiscoveryPublisher& discoveryPublisher, RuntimeManager& runtimeManager, OTAService& otaService,
    I2CBusManager& i2cBusManager,
    const BoardIdentityResolution& boardIdentityResolution,
    BoardProvisioningService& boardProvisioningService,
    ModuleDiscoveryService& moduleDiscoveryService,
    ModuleDescriptorProvisioningService& moduleDescriptorProvisioningService)
    : logger_(logger), configurationService_(configurationService), valueRuntime_(valueRuntime), wifiService_(wifiService),
      mqttService_(mqttService), timeService_(timeService), localeFormatter_(localeFormatter),
      sensorManager_(sensorManager), actuatorRuntime_(actuatorRuntime),
      controllerRuntime_(controllerRuntime),
      measurementSnapshotCache_(measurementSnapshotCache),
      discoveryPublisher_(discoveryPublisher), runtimeManager_(runtimeManager), deviceRestart_(runtimeManager), otaService_(otaService),
      i2cBusManager_(i2cBusManager),
      boardIdentityResolution_(boardIdentityResolution),
      boardProvisioningService_(boardProvisioningService),
      moduleDiscoveryService_(moduleDiscoveryService),
      moduleDescriptorProvisioningService_(moduleDescriptorProvisioningService),
      webHost_(logger, wifiService), server_(webHost_.server()),
      networkRoutes_(webHost_, configurationService, wifiService, *this, *this),
      logRoutes_(webHost_, *this, logReader) {}

void WebService::begin() {
    webHost_.registerRoutes(*this);
    webHost_.registerRoutes(networkRoutes_);
    webHost_.registerRoutes(logRoutes_);
    if (!firmwareRoutes_) {
        firmwareRoutes_.reset(new (std::nothrow) FirmwareWebRoutes(webHost_.server(), *this, otaService_, runtimeManager_,
          {FirmwareBuildInfo::SemanticVersion, FirmwareBuildInfo::CompactIdentity,
           FirmwareBuildInfo::BuildNumber, FirmwareBuildInfo::GitCommit,
           FirmwareBuildInfo::GitBranch, FirmwareBuildInfo::BuildTimestampUtc,
           FirmwareBuildInfo::SourceState, FirmwareBuildInfo::SourceDirty},
          [this]() { return configurationService_.resetToDefaults(); },
          [this](double value, uint8_t decimals) { return localeFormatter_.formatNumber(value, decimals); }));
    }
    if (firmwareRoutes_) webHost_.registerRoutes(*firmwareRoutes_);
    else logger_.error("Insufficient memory to initialize firmware web routes");
    // Keep this administration object out of the ESP32's constrained static DRAM.
    if (!provisioningRoutes_) {
        provisioningRoutes_.reset(new (std::nothrow) HardwareProvisioningWebRoutes(
            server_, *this, logger_, *this,
            [this]() { return renderHardwareOverview(); },
            [](uint8_t* bytes, size_t size) { esp_fill_random(bytes, size); }));
    }
    if (provisioningRoutes_) webHost_.registerRoutes(*provisioningRoutes_);
    else logger_.error("Insufficient memory to initialize provisioning web routes");
    if (!settingsRoutes_) {
        settingsRoutes_.reset(new (std::nothrow) CommonSettingsWebRoutes(webHost_, *this, *this,
            configurationService_, timeService_, localeFormatter_,
            [this]() { return renderDeviceExtras(); },
            [this](bool success, bool localeChanged, bool timeChanged) {
                sendConfigurationResult(configurationSaveResult(success, ConfigurationArea::Locale,
                    localeChanged, ConfigurationArea::Time, timeChanged),
                    uiText(TextId::LocaleAndTimeSaved), uiText(TextId::LocaleAndTimeSaveFailed),
                    "/time", uiText(TextId::InvalidLocaleOrTimeConfiguration));
            },
            [this](bool success) {
                sendConfigurationResult(configurationSaveResult(success, ConfigurationArea::Device),
                    uiText(TextId::DeviceSettingsSaved), uiText(TextId::DeviceSaveFailed),
                    "/device", uiText(TextId::InvalidDeviceName));
            }));
    }
    if (settingsRoutes_) webHost_.registerRoutes(*settingsRoutes_);
    else logger_.error("Insufficient memory to initialize base settings web routes");
    webHost_.begin();
}

void WebService::registerRoutes(WebServer& server) {
    server.on("/values", HTTP_GET, [this]() { handleValues(); });
    server.on("/values/edit", HTTP_GET, [this]() { handleValueEdit(); });
    server.on("/values/save", HTTP_POST, [this]() { handleValueSave(); });
    server.on("/values/set", HTTP_POST, [this]() { handleValueSet(); });
    server.on("/values/delete", HTTP_POST, [this]() { handleValueDelete(); });
    server.on("/", HTTP_GET, [this]() { handleStatus(); });
    server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/sensors", HTTP_GET, [this]() { handleSensors(); });
    server.on("/actuators", HTTP_GET, [this]() { handleActuators(); });
    server.on("/controllers", HTTP_GET, [this]() { handleControllers(); });
    server.on("/measurements", HTTP_GET, [this]() { handleMeasurements(); });
    server.on("/display", HTTP_GET, [this]() { handleDisplay(); });
    server.on("/display-editor.js", HTTP_GET, [this]() {
        // Stream static code directly from flash; never add it to the page's heap buffer.
        server_.sendHeader("Cache-Control", "no-store");
        server_.send_P(200, PSTR("application/javascript; charset=utf-8"), PageEditorScript, sizeof(PageEditorScript) - 1);
    });
    server.on("/display/outputs/save", HTTP_POST, [this]() { handleDisplayOutputsSave(); });
    server.on("/display/save", HTTP_POST, [this]() { handleDisplaySave(); });
    server.on("/sensors/edit", HTTP_GET, [this]() { handleSensorEdit(); });
    server.on("/actuators/edit", HTTP_GET, [this]() { handleActuatorEdit(); });
    server.on("/controllers/edit", HTTP_GET, [this]() { handleControllerEdit(); });
    server.on("/mqtt", HTTP_GET, [this]() { handleMqtt(); });
    server.on("/units", HTTP_GET, [this]() { handleUnits(); });
    server.on("/configuration", HTTP_GET, [this]() { handleConfiguration(); });
    server.on("/configuration/download", HTTP_POST, [this]() { handleConfigurationDownload(); });
    server.on("/configuration/preview", HTTP_POST,
        [this]() { handleConfigurationPreview(); }, [this]() { handleConfigurationUploadData(); });
    server.on("/configuration/restore", HTTP_POST, [this]() { handleConfigurationRestore(); });
    server.on("/configuration/cancel", HTTP_POST, [this]() {
        restoreCandidate_.reset(); restoreToken_ = String(); handleConfiguration();
    });
    server.on("/diagnostics", HTTP_GET, [this]() { handleDiagnostics(); });
    server.on("/diagnostics/i2c/scan", HTTP_POST, [this]() { handleI2CScan(); });
    server.on("/mqtt/save", HTTP_POST, [this]() { handleMqttSave(); });
    server.on("/mqtt/discovery/republish", HTTP_POST,
        [this]() { handleDiscoveryRepublish(); });
    server.on("/units/save", HTTP_POST, [this]() { handleUnitsSave(); });
    server.on("/pressure/save", HTTP_POST, [this]() { handlePressureReductionSave(); });
    server.on("/location/save", HTTP_POST, [this]() { handleGeoPositionSave(); });
    server.on("/sensors/save", HTTP_POST, [this]() { handleSensorSave(); });
    server.on("/sensors/apply", HTTP_POST, [this]() { handleSensorApply(); });
    server.on("/actuators/save", HTTP_POST, [this]() { handleActuatorSave(); });
    server.on("/actuators/apply", HTTP_POST, [this]() { handleActuatorApply(); });
    server.on("/actuators/on", HTTP_POST, [this]() { handleActuatorOn(); });
    server.on("/actuators/off", HTTP_POST, [this]() { handleActuatorOff(); });
    server.on("/actuators/level", HTTP_POST, [this]() { handleActuatorLevel(); });
    server.on("/controllers/save", HTTP_POST, [this]() { handleControllerSave(); });
    server.on("/controllers/apply", HTTP_POST, [this]() { handleControllerApply(); });
    server.on("/controllers/start", HTTP_POST, [this]() { handleControllerStart(); });
    server.on("/controllers/stop", HTTP_POST, [this]() { handleControllerStop(); });
    server.onNotFound([this]() { handleNotFound(); });
}

void WebService::loop() {
    if (restoreCandidate_ && millis() - restorePreviewTime_ > 300000UL) {
        restoreCandidate_.reset(); restoreToken_ = String();
    }
    webHost_.loop();
}

bool WebService::administrationAvailable() const { return webHost_.administrationAvailable(); }

void WebService::sendPage(const char* title, const char* activeRoute, const String& content,
    int status, bool wideContent) {
    webHost_.sendPage([this, title, activeRoute, wideContent]() {
        return renderPageHeader(title, activeRoute, wideContent);
    }, content, status,
        uiText(TextId::AdministrationUnavailableWhileNetworkIsConnecting),
        uiText(TextId::PageRenderingFailedPleaseRetry));
}


void WebService::sendConfigurationResult(
    const ConfigurationSaveResult& result,
    const char* successTitle,
    const char* failureTitle,
    const char* route,
    const char* failureMessage) {
    if (!result.success) {
        sendResult(failureTitle, route, failureMessage, false);
        return;
    }
    if (result.requiredAction != RuntimeAction::None) {
        runtimeManager_.request(result.requiredAction);
    }
    sendResult(successTitle, route, pendingActionMessage(), true);
}

const char* WebService::pendingActionMessage() const {
    switch (runtimeManager_.pendingAction()) {
        case RuntimeAction::None: return uiText(TextId::ConfigurationSavedChangesAreActive);
        case RuntimeAction::RestartMqtt: return uiText(TextId::ConfigurationSavedMQTTRestartRequired);
        case RuntimeAction::RestartTime: return uiText(TextId::ConfigurationSavedTimeServiceRestartRequired);
        case RuntimeAction::RestartWiFi: return uiText(TextId::ConfigurationSavedWiFiRestartRequired);
        case RuntimeAction::RestartSensorManager: return uiText(TextId::ConfigurationSavedSensorManagerRestartRequired);
        case RuntimeAction::RestartActuatorRuntime: return uiText(TextId::ConfigurationSavedActuatorRuntimeApplyRequired);
        case RuntimeAction::RestartControllerRuntime: return uiText(TextId::ConfigurationSavedControllerRuntimeApplyRequired);
        case RuntimeAction::RestartDevice: return uiText(TextId::ConfigurationSavedDeviceRestartRequired);
        default: return uiText(TextId::ConfigurationSavedRuntimeActionRequired);
    }
}

String WebService::pendingRuntimeActionHtml() const {
    const RuntimeAction action = runtimeManager_.pendingAction();
    if (action == RuntimeAction::None) return String();
    String html;
    html.reserve(180);
    html = (String("<div class='notice'><strong>") + uiText(TextId::RuntimeActionRequired) + "</strong><p>" + uiText(TextId::PendingRuntimeAction) + " " + "");
    switch (action) {
        case RuntimeAction::RestartMqtt: html += uiText(TextId::MQTTRestart); break;
        case RuntimeAction::RestartTime: html += uiText(TextId::TimeServiceRestart); break;
        case RuntimeAction::RestartWiFi: html += uiText(TextId::WiFiRestart); break;
        case RuntimeAction::RestartSensorManager: html += uiText(TextId::SensorManagerRestart); break;
        case RuntimeAction::RestartActuatorRuntime: html += uiText(TextId::ActuatorRuntimeApply); break;
        case RuntimeAction::RestartControllerRuntime: html += uiText(TextId::ControllerRuntimeApply); break;
        case RuntimeAction::RestartDevice: html += uiText(TextId::DeviceRestart); break;
        case RuntimeAction::None: break;
    }
    html += ".</p></div>";
    return html;
}


const char* WebService::uiText(TextId id) const {
    return webText(configurationService_.getLocale(), id);
}

String WebService::renderPageHeader(const char* title, const char* active,
    bool wideContent) const {
    const Configuration& cfg = configurationService_.getConfiguration();
    const WebPageFrameData page = {
        title, cfg.device.name, "EnvNode", FirmwareBuildInfo::SemanticVersion,
        FirmwareBuildInfo::BuildTimestampUtc, webLanguageTag(cfg.locale.locale),
        [this]() { return buildWebUiScript(configurationService_.getLocale()); },
        [this, active]() { return buildWebNavigationHtml(active, configurationService_.getLocale()); },
        [this]() { return pendingRuntimeActionHtml(); },
        wifiService_.inSetupAccessPointMode(),
        uiText(TextId::SetupAccessPointMode), wideContent
    };
    return buildWebPageHeader(page);
}

String WebService::currentLocalDateTime() const {
    tm localTime;
    return timeService_.localCivilTime(localTime)
        ? localeFormatter_.formatDateTime(localTime)
        : String("—");
}

void WebService::handleStatus() {
    const Configuration& cfg = configurationService_.getConfiguration();
    const bool connected = wifiService_.connected();
    const bool setupAccessPoint = wifiService_.inSetupAccessPointMode();
    const char* activeHostname = setupAccessPoint ? WiFi.softAPgetHostname() : WiFi.getHostname();
    const String hostname(activeHostname == nullptr ? "" : activeHostname);
    const String ssid = connected ? WiFi.SSID() : setupAccessPoint ? WiFi.softAPSSID() : String();
    const String ipv4Address = connected ? WiFi.localIP().toString()
        : setupAccessPoint ? WiFi.softAPIP().toString() : String();
    const String subnetMask = connected ? WiFi.subnetMask().toString()
        : setupAccessPoint ? WiFi.softAPSubnetMask().toString() : String();
    const String gateway = connected ? WiFi.gatewayIP().toString()
        : setupAccessPoint ? WiFi.softAPIP().toString() : String();
    const String dns1 = connected ? WiFi.dnsIP(0).toString() : String();
    const String dns2 = connected ? WiFi.dnsIP(1).toString() : String();
    const String macAddress = setupAccessPoint ? WiFi.softAPmacAddress() : WiFi.macAddress();
    String c;
    c.reserve(2300);
    c = (String("<div class='summary-grid primary'><section class='card'><h2>") + uiText(TextId::Device) + "</h2><div class='kv'><span>" + uiText(TextId::Name) + "</span><span>") + escapeHtml(cfg.device.name) + (String("</span><span>") + uiText(TextId::Firmware) + "</span><span>") + FirmwareBuildInfo::SemanticVersion + "</span><span>Build</span><span>" + FirmwareBuildInfo::CompactIdentity + (String("</span><span>") + uiText(TextId::Uptime) + "</span><span>") + localeFormatter_.formatNumber(millis()/1000UL, 0) + (((String("") + String(" ") + uiText(TextId::Seconds) + "</span><span>")) + uiText(TextId::FreeHeap) + "</span><span>") + localeFormatter_.formatNumber(ESP.getFreeHeap(), 0) + (String("") + String(" ") + uiText(TextId::Bytes) + "</span><span>Flash</span><span>") + localeFormatter_.formatNumber(ESP.getFlashChipSize()/1024UL, 0) + " KB</span></div></section>";
    c += (String("<section class='card'><h2>") + uiText(TextId::Network) + (String("</h2><div class='kv'><span>") + uiText(TextId::Status) + "</span><span>")) + (connected?badge(uiText(TextId::Connected),"good"):setupAccessPoint?badge(uiText(TextId::SetupAP),"warn"):badge(uiText(TextId::Disconnected),"bad")) + "</span>";
    c += (String("<span>") + uiText(TextId::ConnectionMode) + "</span><span>") + effectiveConnectionMode(setupAccessPoint, configurationService_.getLocale()) + (String("</span><span>") + uiText(TextId::Hostname) + "</span><span>") + availableValue(hostname) + "</span><span>SSID</span><span>" + availableValue(ssid) + "</span>";
    c += (String("<span>") + uiText(TextId::IPv4Address) + "</span><span>") + availableValue(ipv4Address) + (String("</span><span>") + uiText(TextId::SubnetMask) + "</span><span>") + availableValue(subnetMask) + (String("</span><span>") + uiText(TextId::DefaultGateway) + "</span><span>") + availableValue(gateway) + "</span>";
    c += (String("<span>") + uiText(TextId::DNSServer1) + "</span><span>") + availableValue(dns1) + "</span>";
    if (!dns2.isEmpty() && dns2 != "0.0.0.0") c += (String("<span>") + uiText(TextId::DNSServer2) + "</span><span>") + escapeHtml(dns2) + "</span>";
    c += (String("<span>") + uiText(TextId::MACAddress) + "</span><span>") + availableValue(macAddress) + "</span><span>RSSI</span><span>" + (connected?localeFormatter_.formatNumber(wifiService_.rssi(),0)+" dBm":"—") + "</span></div></section>";
    const bool configured = !cfg.mqtt.server.isEmpty();
    c += (((String("<section class='card'><h2>") + uiText(TextId::MQTT) + "</h2><div class='kv'><span>")) + uiText(TextId::Configuration) + "</span><span>") + badge(configured?uiText(TextId::Configured):uiText(TextId::NotConfigured),configured?"good":"warn") + (String("</span><span>") + uiText(TextId::Runtime) + "</span><span>") + badge(mqttService_.connected()?uiText(TextId::Connected):uiText(TextId::Disconnected),mqttService_.connected()?"good":"bad") + "</span></div></section>";
    c += (String("<section class='card'><h2>") + uiText(TextId::Time) + (String("</h2><div class='kv'><span>") + uiText(TextId::Status) + "</span><span>")) + badge(uiText(timeService_.synchronized()?TextId::Synchronized:TextId::Synchronizing),timeService_.synchronized()?"good":"warn") + (String("</span><span>") + uiText(TextId::LocalTime) + "</span><span>") + (timeService_.synchronized()?escapeHtml(currentLocalDateTime()):"—") + "</span></div></section></div>";
    size_t runningControllerCount = 0;
    for (size_t index = 0; index < controllerRuntime_.runtimeCount(); ++index) {
        ControllerRuntimeInfo info;
        if (controllerRuntime_.runtimeInfo(index, info) && info.running) {
            ++runningControllerCount;
        }
    }
    c += (String("<div class='summary-grid domain'><section class='card domain-summary'><h2>") + uiText(TextId::Sensors) + (String("</h2><div class='kv'><span>") + uiText(TextId::Active) + "</span><span>"))
        + localeFormatter_.formatNumber(sensorManager_.sensorCount(), 0)
        + (String("</span></div><p><a href='/sensors'>") + uiText(TextId::ViewSensors) + "</a></p></section>");
    c += (String("<section class='card domain-summary'><h2>") + uiText(TextId::Actuators) + (String("</h2><div class='kv'><span>") + uiText(TextId::Active) + "</span><span>"))
        + localeFormatter_.formatNumber(actuatorRuntime_.runtimeCount(), 0)
        + (String("</span></div><p><a href='/actuators'>") + uiText(TextId::ViewActuators) + "</a></p></section>");
    c += (String("<section class='card domain-summary'><h2>") + uiText(TextId::Controllers) + (String("</h2><div class='kv'><span>") + uiText(TextId::Active) + "</span><span>"))
        + localeFormatter_.formatNumber(controllerRuntime_.runtimeCount(), 0)
        + (String("</span><span>") + uiText(TextId::Running) + "</span><span>")
        + localeFormatter_.formatNumber(runningControllerCount, 0)
        + (String("</span></div><p><a href='/controllers'>") + uiText(TextId::ViewControllers) + "</a></p></section></div>");
    sendPage(uiText(TextId::Status), "/status", c);
}


void WebService::handleMqtt() {
    const MqttConfiguration& m = configurationService_.getConfiguration().mqtt;
    String c;
    c.reserve(1300);
    c = (String("<section class='card'><h2>") + uiText(TextId::RuntimeStatus) + "</h2>") + (m.server.isEmpty()?badge(uiText(TextId::NotConfigured),"warn"):mqttService_.connected()?badge(uiText(TextId::Connected),"good"):badge(uiText(TextId::Disconnected),"bad")) + (String("</section><section class='card'><h2>") + uiText(TextId::BrokerConfiguration) + "</h2><form method='post' action='/mqtt/save' autocomplete='off'>");
    c += (String("<label>") + uiText(TextId::BrokerHostnameOrIP) + "<input name='server' value='") + escapeHtml(m.server) + (String("'></label><label>") + uiText(TextId::Port) + "<input type='number' min='1' max='65535' name='port' required value='") + String(m.port) + (String("'></label><label>") + uiText(TextId::Username) + "<input name='username' value='") + escapeHtml(m.username) + (String("'></label><label class='choice'><input type='checkbox' name='changePassword' value='1'>") + uiText(TextId::ChangeMQTTPassword) + "</label><label>" + uiText(TextId::NewPassword) + "<input type='password' name='password' autocomplete='new-password'></label><div class='actions'><button>" + uiText(TextId::SaveMQTTSettings) + "</button></div></form></section>");
    c += (String("<section class='card'><h2>Home Assistant Discovery</h2><p class='help'>") + uiText(TextId::RepublishTheCurrentRetainedDeviceDiscoveryConfiguration) + "</p><form method='post' action='/mqtt/discovery/republish'><button type='submit'>" + uiText(TextId::RepublishHomeAssistantDiscovery) + "</button></form></section>");
    sendPage(uiText(TextId::MQTT), "/mqtt", c);
}


void WebService::handleUnits() {
    const PresentationConfiguration& p = configurationService_.getConfiguration().presentation;
    String c;
    c.reserve(1400);
    c = (String("<section class='card'><h2>") + uiText(TextId::PresentationUnits) + "</h2><p class='help'>" + uiText(TextId::ChangesAffectFutureMQTTPayloadsAndDoNot) + "</p><form method='post' action='/units/save'>");
    c += (String("<label>") + uiText(TextId::Temperature) + "") + unitSelect("temperature",MeasurementType::Temperature,p.temperature, configurationService_.getLocale()) + (String("</label><label>") + uiText(TextId::AtmosphericPressure) + "") + unitSelect("pressure",MeasurementType::AtmosphericPressure,p.atmosphericPressure, configurationService_.getLocale()) + (String("</label><label>") + uiText(TextId::SolarCellTemperature) + "") + unitSelect("solarTemperature",MeasurementType::SolarCellTemperature,p.solarCellTemperature, configurationService_.getLocale()) + (String("</label><label>") + uiText(TextId::RainDetectorLevel) + "") + unitSelect("rainLevel",MeasurementType::RainDetectorLevel,p.rainDetectorLevel, configurationService_.getLocale()) + (String("</label><div class='actions'><button>") + uiText(TextId::SaveUnits) + "</button></div></form></section>");
    sendPage(uiText(TextId::Units), "/units", c);
}

void WebService::handleConfiguration() {
    String c = "<section class='card'><h2>" + String(uiText(TextId::BackupDownload))
        + "</h2><p>" + uiText(TextId::BackupHelp)
        + "</p><form method='post' action='/configuration/download'><label class='choice'>"
        + "<input type='checkbox' name='includePasswords' value='1'>" + uiText(TextId::BackupPasswords)
        + "</label><button>" + uiText(TextId::BackupDownload) + "</button></form></section>"
        + "<section class='card'><h2>" + uiText(TextId::RestorePreview) + "</h2><p>"
        + uiText(TextId::RestoreHelp) + "</p><form method='post' action='/configuration/preview' enctype='multipart/form-data' onsubmit='this.action=\"/configuration/preview?configurationSize=\"+this.elements.configuration.files[0].size'>"
        + "<input type='file' name='configuration' accept='.json,application/json' required><div class='actions'><button>"
        + uiText(TextId::RestoreUpload) + "</button></div></form></section>";
    sendPage(uiText(TextId::Configuration), "/configuration", c);
}

void WebService::handleConfigurationDownload() {
    String backup;
    const bool passwords = server_.arg("includePasswords") == "1";
    if (!encodeConfigurationBackup(configurationService_.getConfiguration(), passwords,
            FirmwareBuildInfo::SemanticVersion, backup)) {
        sendResult(uiText(TextId::RestoreFailed), "/configuration", uiText(TextId::BackupInvalid), false); return;
    }
    server_.sendHeader("Cache-Control", "no-store");
    server_.sendHeader("Content-Disposition", "attachment; filename=\"envnode-configuration.json\"");
    server_.send(200, "application/json; charset=utf-8", backup);
}

void WebService::handleConfigurationUploadData() {
    HTTPUpload& upload = server_.upload();
    if (upload.status == UPLOAD_FILE_START) {
        // Multiple files in one request are rejected rather than silently selecting the last one.
        configurationUploadFailed_ = !configurationUpload_.isEmpty() || configurationUploadComplete_;
        restoreCandidate_.reset(); restoreToken_ = String();
        if (otaService_.busy() || upload.name != "configuration") configurationUploadFailed_ = true;
        const String hint = server_.arg("configurationSize");
        const TextUploadResult preparation = configurationUploadFailed_ ? TextUploadResult::InvalidSize
            : prepareTextUpload(configurationUpload_, hint, MaxConfigurationBackupLength);
        configurationUploadFailed_ = configurationUploadFailed_ || preparation != TextUploadResult::Success;
        logger_.infof("Configuration backup upload started field=%s expected=%s result=%s heap=%u largest=%u",
            upload.name.c_str(), hint.isEmpty() ? "unknown" : hint.c_str(), textUploadResultName(preparation),
            static_cast<unsigned>(ESP.getFreeHeap()), static_cast<unsigned>(ESP.getMaxAllocHeap()));
    } else if (upload.status == UPLOAD_FILE_WRITE && !configurationUploadFailed_) {
        const size_t offset = configurationUpload_.length();
        const TextUploadResult result = appendTextUpload(configurationUpload_, upload.buf,
            upload.currentSize, MaxConfigurationBackupLength);
        if (result != TextUploadResult::Success) {
            logger_.warnf("Configuration backup upload rejected result=%s offset=%u chunk=%u heap=%u largest=%u",
                textUploadResultName(result), static_cast<unsigned>(offset), static_cast<unsigned>(upload.currentSize),
                static_cast<unsigned>(ESP.getFreeHeap()), static_cast<unsigned>(ESP.getMaxAllocHeap()));
            configurationUploadFailed_ = true; configurationUpload_ = String();
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        configurationUploadComplete_ = true;
        size_t expected = 0;
        const String hint = server_.arg("configurationSize");
        if (parseTextUploadSize(hint, MaxConfigurationBackupLength, expected) != TextUploadResult::Success
            || (expected && expected != configurationUpload_.length())) configurationUploadFailed_ = true;
        logger_.infof("Configuration backup upload complete bytes=%u rejected=%u heap=%u largest=%u",
            static_cast<unsigned>(configurationUpload_.length()), configurationUploadFailed_ ? 1u : 0u,
            static_cast<unsigned>(ESP.getFreeHeap()), static_cast<unsigned>(ESP.getMaxAllocHeap()));
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        logger_.warn("Configuration backup upload aborted by client");
        configurationUploadFailed_ = false; configurationUploadComplete_ = false; configurationUpload_ = String();
        restoreCandidate_.reset(); restoreToken_ = String();
    }
}

void WebService::handleConfigurationPreview() {
    const uint32_t startedAt = millis();
    restoreCandidate_.reset();
    logger_.infof("Configuration preview decoding bytes=%u heap=%u largest=%u",
        static_cast<unsigned>(configurationUpload_.length()),
        static_cast<unsigned>(ESP.getFreeHeap()), static_cast<unsigned>(ESP.getMaxAllocHeap()));
    bool valid = !otaService_.busy() && !configurationUploadFailed_ && configurationUploadComplete_
        && decodeConfigurationBackupCandidate(configurationUpload_,
            configurationService_.getConfiguration(), restoreCandidate_, restoreIncludesPasswords_);
    // The candidate owns its strings. Release the uploaded JSON before domain
    // validation and rendering, instead of keeping both representations alive.
    configurationUpload_ = String(); configurationUploadFailed_ = false; configurationUploadComplete_ = false;
    logger_.infof("Configuration preview decoded=%u elapsed=%u heap=%u largest=%u",
        valid ? 1u : 0u, static_cast<unsigned>(millis() - startedAt),
        static_cast<unsigned>(ESP.getFreeHeap()), static_cast<unsigned>(ESP.getMaxAllocHeap()));
    if (valid) valid = configurationService_.validateRestore(*restoreCandidate_);
    logger_.infof("Configuration preview validated=%u elapsed=%u", valid ? 1u : 0u,
        static_cast<unsigned>(millis() - startedAt));
    if (!valid) {
        restoreCandidate_.reset(); restoreToken_ = String();
        sendResult(uiText(TextId::RestoreFailed), "/configuration", uiText(TextId::BackupInvalid), false); return;
    }
    restorePreviewTime_ = millis();
    restoreToken_ = String(++restoreSequence_) + "-" + String(restorePreviewTime_);
    const auto& c = *restoreCandidate_;
    unsigned sensors = 0, actuators = 0, controllers = 0;
    for (const auto& s : c.sensorSlots) sensors += s.enabled;
    for (const auto& s : c.actuatorSlots) actuators += s.enabled;
    for (const auto& s : c.controllerSlots) controllers += s.enabled;
    String html = "<section class='card'><h2>" + String(uiText(TextId::RestorePreview))
        + "</h2><p>" + uiText(TextId::RestoreWarning) + "</p><div class='kv'><span>"
        + uiText(TextId::DeviceName) + "</span><span>" + escapeHtml(c.device.name)
        + "</span><span>" + uiText(TextId::Network) + "</span><span>" + escapeHtml(c.network.hostname)
        + " / " + escapeHtml(c.network.wifiSSID)
        + (c.network.addressMode == NetworkAddressMode::Static ? " / " + escapeHtml(c.network.ipv4Address) : " / DHCP")
        + "</span><span>MQTT</span><span>" + escapeHtml(c.mqtt.server) + ":" + String(c.mqtt.port)
        + "</span><span>" + uiText(TextId::Latitude) + "</span><span>"
        + (c.geoPosition.latitude.configured ? formatDecimalInput(c.geoPosition.latitude.value, 7, configurationService_.getLocale()) : String("—"))
        + "</span><span>" + uiText(TextId::Longitude) + "</span><span>"
        + (c.geoPosition.longitude.configured ? formatDecimalInput(c.geoPosition.longitude.value, 7, configurationService_.getLocale()) : String("—"))
        + "</span><span>" + uiText(TextId::Altitude) + "</span><span>"
        + (c.geoPosition.altitude.configured ? formatDecimalInput(c.geoPosition.altitude.value, 3, configurationService_.getLocale()) : String("—"))
        + "</span><span>" + uiText(TextId::PressureTemperatureSource) + "</span><span>"
        + (c.pressureReduction.temperatureSensorId == InvalidSensorId ? String(uiText(TextId::PressureNoTemperature))
            : String(uiText(TextId::Slot)) + " " + String(c.pressureReduction.temperatureSensorId))
        + "</span><span>" + uiText(TextId::PressureTemperatureAge) + "</span><span>"
        + formatDecimalInput(c.pressureReduction.maxTemperatureAgeMs / 1000.0, 3, configurationService_.getLocale())
        + "</span><span>" + uiText(TextId::Sensors) + "</span><span>" + String(sensors)
        + "</span><span>" + uiText(TextId::Actuators) + "</span><span>" + String(actuators)
        + "</span><span>" + uiText(TextId::Controllers) + "</span><span>" + String(controllers)
        + "</span></div><p>" + uiText(restoreIncludesPasswords_ ? TextId::PasswordsRestored : TextId::PasswordsPreserved)
        + "</p><form method='post' action='/configuration/restore'><input type='hidden' name='token' value='"
        + restoreToken_ + "'><label class='choice'><input type='checkbox' name='confirmRestore' value='1' required>"
        + uiText(TextId::RestoreConfirm) + "</label><button class='danger'>" + uiText(TextId::RestoreConfirm)
        + "</button></form><form method='post' action='/configuration/cancel'><button>"
        + uiText(TextId::CancelRestore) + "</button></form></section>";
    server_.sendHeader("Cache-Control", "no-store");
    sendPage(uiText(TextId::RestorePreview), "/configuration", html);
    logger_.infof("Configuration preview response sent elapsed=%u heap=%u largest=%u",
        static_cast<unsigned>(millis() - startedAt), static_cast<unsigned>(ESP.getFreeHeap()),
        static_cast<unsigned>(ESP.getMaxAllocHeap()));
}

void WebService::handleConfigurationRestore() {
    logger_.info("Configuration restore confirmation received");
    if (otaService_.busy() || !restoreCandidate_ || server_.arg("confirmRestore") != "1"
        || server_.arg("token") != restoreToken_ || millis() - restorePreviewTime_ > 300000UL) {
        logger_.warn("Configuration restore confirmation rejected: missing, expired or mismatched preview");
        sendResult(uiText(TextId::RestoreFailed), "/configuration", uiText(TextId::RestoreExpired), false); return;
    }
    if (!restoreIncludesPasswords_) {
        const auto& current = configurationService_.getConfiguration();
        restoreCandidate_->network.wifiPassword = current.network.wifiPassword;
        restoreCandidate_->mqtt.password = current.mqtt.password;
    }
    const bool saved = configurationService_.restoreConfiguration(*restoreCandidate_);
    logger_.info(saved ? "Configuration restore saved and verified; device restart required"
        : "Configuration restore rejected or persistence failed; previous configuration retained");
    restoreCandidate_.reset(); restoreToken_ = String();
    if (!saved) {
        sendResult(uiText(TextId::RestoreFailed), "/configuration", uiText(TextId::RestoreSaveFailed), false); return;
    }
    deviceRestart_.requestDeviceRestart();
    String html = "<div class='notice success'><strong>" + String(uiText(TextId::RestoreSaved))
        + "</strong><p>" + uiText(TextId::RestoreRestart)
        + "</p></div><form method='post' action='/restart' onsubmit='return confirm(globalThis.EnvNodeUi.RestartTheDeviceNow)'><button>"
        + uiText(TextId::RestartNow) + "</button></form><a class='button' href='/configuration'>"
        + uiText(TextId::RestartLater) + "</a>";
    sendPage(uiText(TextId::RestoreSaved), "/configuration", html);
}


void WebService::handleSensors() {
    String c;
    c.reserve(1200 + MaxSensorSlotCount * 400);
    if (runtimeManager_.pendingAction() == RuntimeAction::RestartSensorManager) {
        c = (String("<div class='notice'><strong>") + uiText(TextId::SensorRestartRequired) + "</strong><p>" + uiText(TextId::SavedSensorConfigurationDiffersFromTheActiveRuntime) + "</p><form method='post' action='/sensors/apply'><button>" + uiText(TextId::ApplySensorChanges) + "</button></form></div>");
    }
    c += (((String("<section class='card'><h2>") + uiText(TextId::SensorSlots) + "</h2><p class='help'>" + uiText(TextId::SavingAndRuntimeActivationAreSeparateActionsEnabled) + "</p><div class='scroll'><table><thead><tr><th class='sensor-technical'>" + uiText(TextId::Slot) + "</th><th>")) + uiText(TextId::Name) + "</th><th>" + uiText(TextId::Configured) + (String("</th><th class='sensor-technical'>") + uiText(TextId::Connection) + "</th><th class='sensor-technical'>" + uiText(TextId::Schedule) + "</th><th>" + uiText(TextId::Runtime) + "</th><th class='sensor-technical'>") + uiText(TextId::State) + (String("</th><th class='sensor-last-measurement'>") + uiText(TextId::LastMeasurement) + "</th><th class='sensor-actions'></th></tr></thead><tbody>"));
    const Configuration& configuration = configurationService_.getConfiguration();
    for (size_t slotIndex = 0; slotIndex < MaxSensorSlotCount; ++slotIndex) {
        const SensorSlotConfiguration& slot = configuration.sensorSlots[slotIndex];
        const SensorImplementationMetadata* metadata = SensorImplementationRegistry::find(slot.implementation);
        SensorRuntimeInfo runtime;
        bool hasRuntime = false;
        for (size_t runtimeIndex = 0; runtimeIndex < sensorManager_.sensorCount(); ++runtimeIndex) {
            SensorRuntimeInfo candidate;
            if (sensorManager_.runtimeInfo(runtimeIndex, candidate) && candidate.id == slot.slotId) {
                runtime = candidate;
                hasRuntime = true;
                break;
            }
        }
        c += "<tr><td class='sensor-technical'>" + String(slot.slotId) + "</td><td>" + escapeHtml(slot.name) + "</td><td>";
        c += slot.enabled ? badge(uiText(TextId::Enabled), "good") : badge(uiText(TextId::Disabled), "warn");
        c += "<br>" + escapeHtml(metadata == nullptr ? uiText(TextId::Invalid) : webSensorImplementationName(configurationService_.getLocale(), metadata->implementation));
        c += "</td><td class='sensor-technical'>" + configuredHardwareAssignment(slot.hardware, configurationService_.getLocale()) + "</td><td class='sensor-technical'>";
        c += slot.schedule.acquisitionMode == AcquisitionMode::Periodic
            ? String(slot.schedule.sampleIntervalMs) + " ms" : uiText(TextId::EventOnly);
        c += "</td><td>";
        c += hasRuntime ? escapeHtml(runtime.type) + " / " + hardwareAssignment(runtime, configurationService_.getLocale()) : uiText(TextId::NoRuntimeSensor);
        const bool expectsRuntime = slot.enabled && slot.implementation != SensorImplementation::None;
        const bool runtimeMatches = expectsRuntime == hasRuntime
            && (!hasRuntime || (runtime.implementation == slot.implementation
                && String(runtime.name) == slot.name
                && runtime.schedule.acquisitionMode == slot.schedule.acquisitionMode
                && runtime.schedule.sampleIntervalMs == slot.schedule.sampleIntervalMs
                && sameHardwareAssignment(runtime.hardware, slot.hardware)));
        if (!runtimeMatches) c += "<br>" + badge(uiText(TextId::SensorRestartRequired), "warn");
        c += "</td><td class='sensor-technical'>" + String(hasRuntime ? sensorStateName(runtime.state, configurationService_.getLocale()) : "—") + "</td><td class='sensor-last-measurement'>";
        SensorRuntimeStatus runtimeStatus;
        c += hasRuntime && sensorManager_.runtimeStatus(runtime.id, runtimeStatus)
            ? lastMeasurementDisplay(runtimeStatus, localeFormatter_, configurationService_.getLocale())
            : String("—");
        c += "</td>";
        c += "<td class='sensor-actions'><a class='button' href='/sensors/edit?slot=" + String(slot.slotId) + (String("'>") + uiText(TextId::Configure) + "</a></td></tr>");
    }
    c += "</tbody></table></div></section>";
    sendPage(uiText(TextId::Sensors), "/sensors", c);
}

void WebService::handleActuators() {
    String c;
    c.reserve(1300 + MaxActuatorSlotCount * 520);
    if (runtimeManager_.pendingAction() == RuntimeAction::RestartActuatorRuntime) {
        c = (String("<div class='notice'><strong>") + uiText(TextId::ActuatorApplyRequired) + "</strong><p>" + uiText(TextId::SavedActuatorConfigurationDiffersFromTheActiveRuntime) + "</p><form method='post' action='/actuators/apply'><button>" + uiText(TextId::ApplyActuatorChanges) + "</button></form></div>");
    }
    c += (((String("<section class='card'><h2>") + uiText(TextId::ActuatorSlots) + "</h2><p class='help'>" + uiText(TextId::SavedConfigurationIsActivatedWithApplyActuatorChanges) + "</p><div class='table-scroll actuator-table-wrap'><table class='actuator-table'><thead><tr><th class='actuator-slot'>" + uiText(TextId::Slot) + "</th><th class='actuator-name'>")) + uiText(TextId::Name) + "</th><th class='actuator-configured'>" + uiText(TextId::Configured) + "</th><th class='actuator-hardware'>" + uiText(TextId::Hardware) + (String("</th><th class='actuator-runtime'>") + uiText(TextId::Runtime) + "</th><th class='actuator-initialization'>" + uiText(TextId::Initialization) + "</th><th class='actuator-state'>") + uiText(TextId::State) + (String("</th><th class='actuator-controls'>") + uiText(TextId::Controls) + "</th><th class='actuator-configure'></th></tr></thead><tbody>"));
    const Configuration& configuration = configurationService_.getConfiguration();
    for (size_t slotIndex = 0; slotIndex < MaxActuatorSlotCount; ++slotIndex) {
        const ActuatorSlotConfiguration& slot = configuration.actuatorSlots[slotIndex];
        const ActuatorImplementationMetadata* metadata =
            ActuatorImplementationRegistry::find(slot.implementation);
        ActuatorRuntimeInfo runtime;
        bool hasRuntime = false;
        for (size_t runtimeIndex = 0; runtimeIndex < actuatorRuntime_.runtimeCount(); ++runtimeIndex) {
            ActuatorRuntimeInfo candidate;
            if (actuatorRuntime_.runtimeInfo(runtimeIndex, candidate)
                && candidate.id == slot.slotId) {
                runtime = candidate;
                hasRuntime = true;
                break;
            }
        }
        const bool expectsRuntime = slot.enabled
            && slot.implementation != ActuatorImplementation::None;
        const bool descriptorRuntime = hasRuntime
            && runtime.origin == ActuatorRuntimeOrigin::ModuleDescriptor;
        const bool savedModule = validModuleActuatorReference(slot.moduleTarget);
        const bool module = savedModule || descriptorRuntime;
        ModuleActuatorReference moduleReference = slot.moduleTarget;
        if (!savedModule && descriptorRuntime)
            actuatorRuntime_.moduleReference(slot.slotId, moduleReference);
        const bool runtimeMatches = (savedModule && !hasRuntime)
            || (descriptorRuntime && !savedModule) || (expectsRuntime == hasRuntime
            && (!hasRuntime || (runtime.implementation == slot.implementation
                && String(runtime.name) == slot.name
                && (savedModule || sameHardwareAssignment(runtime.hardware, slot.hardware)))));
        IOnOffActuator* onOff = actuatorRuntime_.onOffActuator(slot.slotId);
        ILevelActuator* level = actuatorRuntime_.levelActuator(slot.slotId);

        c += "<tr><td class='actuator-slot'>" + String(slot.slotId)
            + "</td><td class='actuator-name'>" + escapeHtml(descriptorRuntime && !savedModule ? String(runtime.name) : slot.name)
            + "</td><td class='actuator-configured'>";
        c += (descriptorRuntime && !savedModule ? true : slot.enabled) ? badge(uiText(TextId::Enabled), "good") : badge(uiText(TextId::Disabled), "warn");
        c += "<br>" + escapeHtml(module ? uiText(TextId::ModuleOnOff) : metadata == nullptr ? uiText(TextId::Invalid) : webActuatorImplementationName(configurationService_.getLocale(), metadata->implementation));
        c += "</td><td class='actuator-hardware'>"
            + (module ? escapeHtml(moduleActuatorLabel(moduleDiscoveryService_, moduleReference, configurationService_.getLocale())) : configuredHardwareAssignment(slot.hardware, configurationService_.getLocale())) + "</td><td class='actuator-runtime'>";
        if (hasRuntime) {
            c += escapeHtml(runtime.name) + " / "
                + configuredHardwareAssignment(runtime.hardware, configurationService_.getLocale());
            if (descriptorRuntime) {
                c += "<br>" + badge(uiText(TextId::ModuleDescriptor), "good")
                    + (String("<span class='secondary'>") + uiText(TextId::Slot) + " " + "")
                    + escapeHtml(moduleSlotName(runtime.moduleSlot)) + " / "
                    + escapeHtml(runtime.descriptorDeviceId) + "</span>";
            }
        } else {
            c += savedModule ? uiText(TextId::ModuleDisabledOrUnavailable) : uiText(TextId::NoRuntimeActuator);
        }
        if (!runtimeMatches) c += "<br>" + badge(uiText(TextId::ActuatorApplyRequired), "warn");
        c += "</td><td class='actuator-initialization'>";
        if (!hasRuntime) c += "—";
        else if (runtime.available) c += badge(uiText(TextId::Initialized), "good");
        else if (runtime.initializationAttempted) c += badge(uiText(TextId::Failed), "bad");
        else c += badge(uiText(TextId::ConstructionFailed), "bad");
        c += "</td><td class='actuator-state'>";
        c += onOff == nullptr ? "—" : onOffStateName(onOff->state(), configurationService_.getLocale());
        if (level != nullptr) {
            c += (String("<span class='secondary'>") + uiText(TextId::Level) + " " + "")
                + String(level->level().percent()) + " %</span>";
        }
        c += "</td><td class='actuator-controls'>";
        if (onOff != nullptr) {
            c += "<div class='table-actions vertical'><form method='post' action='/actuators/on'><input type='hidden' name='slot' value='"
                + String(slot.slotId) + (String("'><button type='submit'>") + uiText(TextId::On) + "</button></form>")
                + "<form method='post' action='/actuators/off'><input type='hidden' name='slot' value='"
                + String(slot.slotId) + (String("'><button type='submit'>") + uiText(TextId::Off) + "</button></form></div>");
            if (level != nullptr) {
                c += "<form method='post' action='/actuators/level'><input type='hidden' name='slot' value='"
                    + String(slot.slotId) + (String("'><label class='secondary'>") + uiText(TextId::Level2) + "<input type='number' name='level' min='0' max='100' required value='")
                    + String(level->level().percent())
                    + (String("'></label><button type='submit'>") + uiText(TextId::SetLevel) + "</button></form>");
            }
        } else {
            c += "—";
        }
        c += "</td><td class='actuator-configure'><a class='button table-action' href='/actuators/edit?slot="
            + String(slot.slotId) + (String("'>") + uiText(TextId::Configure) + "</a></td></tr>");
    }
    c += "</tbody></table></div></section>";
    sendPage(uiText(TextId::Actuators), "/actuators", c, 200, true);
}

void WebService::handleControllers() {
    String c;
    c.reserve(1500 + MaxControllerSlotCount * 900);
    if (runtimeManager_.pendingAction() == RuntimeAction::RestartControllerRuntime) {
        c = (String("<div class='notice'><strong>") + uiText(TextId::ControllerApplyRequired) + "</strong><p>" + uiText(TextId::SavedControllerConfigurationDiffersFromTheActiveRuntime) + "</p><form method='post' action='/controllers/apply'><button>" + uiText(TextId::ApplyControllerChanges) + "</button></form></div>");
    }
    c += (((String("<section class='card'><h2>") + uiText(TextId::ControllerSlots) + "</h2><p class='help'>" + uiText(TextId::SavedConfigurationIsActivatedWithApplyControllerChanges) + "</p><div class='table-scroll controller-table-wrap'><table class='controller-table'><thead><tr><th class='controller-slot'>" + uiText(TextId::Slot) + "</th><th class='controller-name'>")) + uiText(TextId::Controller) + (String("</th><th class='controller-route'>") + uiText(TextId::SourceTarget) + "</th><th class='controller-policy'>" + uiText(TextId::Policy) + "</th><th class='controller-runtime'>" + uiText(TextId::Runtime) + "</th><th class='controller-diagnostics'>") + uiText(TextId::Diagnostics) + (String("</th><th class='controller-controls'>") + uiText(TextId::Controls) + "</th></tr></thead><tbody>"));
    const Configuration& configuration = configurationService_.getConfiguration();
    for (size_t slotIndex = 0; slotIndex < MaxControllerSlotCount; ++slotIndex) {
        const ControllerSlotConfiguration& slot = configuration.controllerSlots[slotIndex];
        const ControllerImplementationMetadata* metadata =
            ControllerImplementationRegistry::find(slot.implementation);
        ControllerRuntimeInfo runtime;
        bool hasRuntime = false;
        for (size_t runtimeIndex = 0; runtimeIndex < controllerRuntime_.runtimeCount(); ++runtimeIndex) {
            ControllerRuntimeInfo candidate;
            if (controllerRuntime_.runtimeInfo(runtimeIndex, candidate)
                && candidate.id == slot.slotId) {
                runtime = candidate;
                hasRuntime = true;
                break;
            }
        }
        const BlinkControllerConfiguration& blink =
            slot.implementationConfiguration.blink;
        const ThresholdControllerConfiguration& threshold =
            slot.implementationConfiguration.threshold;
        String moduleTargetName;
        if (validModuleActuatorReference(slot.moduleTarget)) {
            for (size_t actuatorIndex = 0;
                 actuatorIndex < actuatorRuntime_.runtimeCount(); ++actuatorIndex) {
                ActuatorRuntimeInfo actuatorInfo;
                ModuleActuatorReference reference;
                if (actuatorRuntime_.runtimeInfo(actuatorIndex, actuatorInfo)
                    && actuatorRuntime_.moduleReference(actuatorInfo.id, reference)
                    && sameModuleActuatorReference(slot.moduleTarget, reference)) {
                    moduleTargetName = actuatorInfo.name;
                    break;
                }
            }
        }
        const bool expectsRuntime = slot.enabled
            && slot.implementation != ControllerImplementation::None;
        bool runtimeMatches = expectsRuntime == hasRuntime;
        if (runtimeMatches && hasRuntime) {
            runtimeMatches = runtime.implementation == slot.implementation
                && String(runtime.name) == slot.name;
            if (runtimeMatches && slot.implementation == ControllerImplementation::Blink) {
                runtimeMatches = runtime.targetActuatorId == blink.targetActuatorId
                    && runtime.onDurationMs == blink.onDurationMs
                    && runtime.offDurationMs == blink.offDurationMs;
            } else if (runtimeMatches
                && slot.implementation == ControllerImplementation::Threshold) {
                runtimeMatches = runtime.sourceSensorId == threshold.source.sensorId
                    && runtime.sourceMeasurementType == threshold.source.measurementType
                    && runtime.targetActuatorId == threshold.targetActuatorId
                    && runtime.onThreshold == threshold.onThreshold
                    && runtime.offThreshold == threshold.offThreshold
                    && runtime.thresholdDirection == threshold.direction
                    && runtime.maxMeasurementAgeMs == threshold.maxMeasurementAgeMs
                    && runtime.decisionOnly == threshold.decisionOnly;
            }
        }

        if (runtimeMatches && hasRuntime && slot.implementation == ControllerImplementation::Selector) {
            const auto& selector = slot.implementationConfiguration.selector;
            runtimeMatches = runtime.modeValueId == selector.modeValueId
                && runtime.automaticControllerId == selector.automaticControllerId
                && runtime.targetActuatorId == selector.targetActuatorId
                && runtime.selectorMapping == selectorMappingSignature(selector);
        }
        c += "<tr><td class='controller-slot'>" + String(slot.slotId)
            + "</td><td class='controller-name'>" + escapeHtml(slot.name) + "<br>";
        c += slot.enabled ? badge(uiText(TextId::Enabled), "good") : badge(uiText(TextId::Disabled), "warn");
        c += "<span class='secondary'>" + escapeHtml(
            metadata == nullptr ? uiText(TextId::Invalid) : webControllerImplementationName(configurationService_.getLocale(), metadata->implementation)) + "</span>";
        c += "</td><td class='controller-route'>";
        if (slot.implementation == ControllerImplementation::Blink) {
            if (validModuleActuatorReference(slot.moduleTarget)) {
                c += (String(uiText(TextId::TargetModuleActuator)) + " ")
                    + (moduleTargetName.isEmpty()
                        ? String("currently unavailable") : escapeHtml(moduleTargetName));
            } else {
                c += (String(uiText(TextId::TargetActuator)) + " ") + String(blink.targetActuatorId);
            }
            if (!validModuleActuatorReference(slot.moduleTarget)
                && isValidActuatorId(blink.targetActuatorId)
                && blink.targetActuatorId <= MaxActuatorSlotCount) {
                c += " · " + escapeHtml(configuration.actuatorSlots[
                    blink.targetActuatorId - 1].name);
            }
        } else if (slot.implementation == ControllerImplementation::Threshold) {
            c += String(threshold.direction == ThresholdDirection::OnAbove
                ? uiText(TextId::OnAboveOffBelow) : uiText(TextId::OnBelowOffAbove));
            c += "<br>";
            c += (String(uiText(TextId::SourceSensor2)) + " ") + String(threshold.source.sensorId);
            if (isValidSensorId(threshold.source.sensorId)
                && threshold.source.sensorId <= MaxSensorSlotCount) {
                c += " · " + escapeHtml(configuration.sensorSlots[
                    threshold.source.sensorId - 1].name);
            }
            c += (String("<br>") + uiText(TextId::Measurement2) + " " + "");
            c += escapeHtml(webMeasurementName(configurationService_.getLocale(), threshold.source.measurementType));
            if (threshold.decisionOnly) {
                c += (String("<br>") + uiText(TextId::DecisionOnlyNoActuator) + "");
            } else if (validModuleActuatorReference(slot.moduleTarget)) {
                c += (String("<br>") + uiText(TextId::TargetModuleActuator) + " " + "")
                    + (moduleTargetName.isEmpty()
                        ? String("currently unavailable") : escapeHtml(moduleTargetName));
            } else {
                c += (String("<br>") + uiText(TextId::TargetActuator) + " " + "") + String(threshold.targetActuatorId);
            }
            if (!threshold.decisionOnly && !validModuleActuatorReference(slot.moduleTarget)
                && isValidActuatorId(threshold.targetActuatorId)
                && threshold.targetActuatorId <= MaxActuatorSlotCount) {
                c += " · " + escapeHtml(configuration.actuatorSlots[
                    threshold.targetActuatorId - 1].name);
            }
        } else if (slot.implementation == ControllerImplementation::Selector) {
            const auto& selector = slot.implementationConfiguration.selector;
            c += (String(uiText(TextId::ModeValue2)) + " ") + String(selector.modeValueId) + (String("<br>") + uiText(TextId::AutoController) + " " + "")
                + String(selector.automaticControllerId) + (String("<br>") + uiText(TextId::Target2) + " " + "")
                + (validModuleActuatorReference(slot.moduleTarget) ? String(uiText(TextId::ModuleActuator)) : (String(uiText(TextId::Actuator)) + " ") + String(selector.targetActuatorId));
        } else {
            c += "—";
        }
        c += "</td><td class='controller-policy'>";
        if (slot.implementation == ControllerImplementation::Blink) {
            c += String(blink.onDurationMs) + (String("") + String(" ") + uiText(TextId::MsOn) + "<br>") + String(blink.offDurationMs) + (String(" ") + uiText(TextId::MsOff));
        } else if (slot.implementation == ControllerImplementation::Threshold) {
            const char* unit = UnitConverter::symbol(
                measurementTypeMetadata(threshold.source.measurementType).canonicalUnit);
            if (unit == nullptr) unit = "";
            c += (String(uiText(TextId::On)) + " ") + String(thresholdOnComparisonSymbol(threshold.direction))
                + " " + String(threshold.onThreshold, 4) + " " + unit;
            c += (String("<br>") + uiText(TextId::Off) + " " + "") + String(thresholdOffComparisonSymbol(threshold.direction))
                + " " + String(threshold.offThreshold, 4) + " " + unit;
            c += (String("<br>") + uiText(TextId::MaxAge) + " " + "") + String(threshold.maxMeasurementAgeMs) + " ms";
        } else if (slot.implementation == ControllerImplementation::Selector) {
            const auto& selector = slot.implementationConfiguration.selector;
            c += (String(uiText(TextId::Auto)) + " ") + escapeHtml(selector.automaticCode) + (String("<br>") + uiText(TextId::On2) + " " + "") + escapeHtml(selector.onCode)
                + (String("<br>") + uiText(TextId::Off2) + " " + "") + escapeHtml(selector.offCode) + (String("<br>") + uiText(TextId::UnknownHold) + "");
        } else {
            c += "—";
        }
        c += "</td><td class='controller-runtime'>";
        if (!hasRuntime) {
            c += uiText(TextId::NoRuntimeController);
        } else {
            c += runtime.running ? badge(uiText(TextId::Running), "good") : badge(uiText(TextId::Stopped), "warn");
            c += (String("<span class='secondary'>") + uiText(TextId::ActiveRuntime) + "</span>");
        }
        if (!runtimeMatches) c += "<br>" + badge(uiText(TextId::ControllerApplyRequired), "warn");
        c += "</td><td class='controller-diagnostics'><div class='diagnostic-stack'>";
        if (!hasRuntime) {
            c += "—";
        } else if (runtime.implementation == ControllerImplementation::Blink) {
            c += runtime.targetAvailable ? uiText(TextId::TargetAvailable) : uiText(TextId::TargetUnavailable);
            c += "<br>" + String(blinkPhaseName(runtime.blinkPhase, configurationService_.getLocale()));
            c += "<br>" + String(controllerOperationName(runtime.lastOperationResult, configurationService_.getLocale()));
        } else if (runtime.implementation == ControllerImplementation::Threshold) {
            if (!runtime.hasLatestSnapshot) {
                c += uiText(TextId::NoCurrentMeasurement);
            } else if (runtime.latestSnapshotStale) {
                c += uiText(TextId::StaleMeasurement)
                    + formatElapsedDuration(runtime.latestSnapshotAgeMs) + (String(" ") + uiText(TextId::Ago2));
            } else if (!runtime.latestMeasurementValid) {
                c += uiText(TextId::InvalidMeasurement);
            } else if (!runtime.sourceAvailable) {
                c += uiText(TextId::UnusableMeasurement);
            } else {
                c += uiText(TextId::UsableMeasurement);
                if (runtime.latestNumericValueAvailable) {
                    c += ": " + String(runtime.latestNumericValue, 4);
                }
            }
            c += (String("<br>") + uiText(TextId::Decision) + " " + "");
            c += runtime.thresholdDecision == ThresholdDecision::Unknown
                ? badge(uiText(TextId::Unknown), "")
                : runtime.thresholdDecision == ThresholdDecision::On
                    ? badge(uiText(TextId::On), "good") : badge(uiText(TextId::Off), "warn");
            c += "<br>";
            c += runtime.decisionOnly ? uiText(TextId::DecisionOnly) : runtime.targetAvailable ? uiText(TextId::TargetAvailable) : uiText(TextId::TargetUnavailable);
            if (runtime.outputApplicationPending) {
                c += "<br>" + badge(uiText(TextId::OutputApplicationPending), "warn");
            }
            c += "<br>" + String(controllerOperationName(runtime.lastOperationResult, configurationService_.getLocale()));
        } else if (runtime.implementation == ControllerImplementation::Selector) {
            c += runtime.thresholdDecision == ThresholdDecision::Unknown ? uiText(TextId::HoldingOutputNoDecision)
                : runtime.thresholdDecision == ThresholdDecision::On ? uiText(TextId::SelectedOn) : uiText(TextId::SelectedOff);
            c += runtime.targetAvailable ? (String("<br>") + uiText(TextId::TargetAvailable) + "") : (String("<br>") + uiText(TextId::TargetUnavailable) + "");
            if (runtime.outputApplicationPending) c += (String("<br>") + uiText(TextId::OutputApplicationPending) + "");
            c += "<br>" + String(controllerOperationName(runtime.lastOperationResult, configurationService_.getLocale()));
        } else {
            c += uiText(TextId::UnsupportedRuntimeImplementation);
        }
        c += "</div></td><td class='controller-controls'>";
        c += "<div class='table-actions vertical'>";
        if (hasRuntime) {
            const char* action = runtime.running ? "stop" : "start";
            const char* label = runtime.running ? uiText(TextId::Stop) : uiText(TextId::Start);
            c += "<form method='post' action='/controllers/" + String(action)
                + "'><input type='hidden' name='slot' value='" + String(slot.slotId)
                + "'><button type='submit'>" + label + "</button></form>";
        }
        c += "<a class='button table-action' href='/controllers/edit?slot="
            + String(slot.slotId) + (String("'>") + uiText(TextId::Configure) + "</a></div></td></tr>");
    }
    c += "</tbody></table></div></section>";
    sendPage(uiText(TextId::Controllers), "/controllers", c, 200, true);
}

bool WebService::readDisplayEnumTranslations(DisplayPage& page, const IPropertyReader& properties) {
    page.enumTranslations.clear();
    for (size_t line = 0; line < DisplayLineCount; ++line) for (size_t source = 0; source < MaxPropertySourcesPerLine; ++source) {
        PropertySourceInput parsed;
        PropertyDescription description;
        if (!parsePropertySource(page.sources[line][source].c_str(), parsed)
            || !properties.describe(parsed.reference(), description)
            || description.valueKind != PropertyValueKind::Enumeration) continue;
        for (size_t index = 0; index < description.enumOptionCount; ++index) {
            const auto& option = description.enumOptions[index];
            const String field = "e" + String(static_cast<unsigned int>(line)) + "_"
                + String(static_cast<unsigned int>(source)) + "_" + option.stableCode;
            const String text = server_.arg(field);
            if (text.isEmpty()) continue;
            if (page.enumTranslations.size() == MaxDisplayEnumTranslations || !validateDisplayBooleanLabel(text)) return false;
            DisplayEnumTranslation entry;
            entry.line = line; entry.source = source; entry.code = option.stableCode; entry.text = text;
            page.enumTranslations.push_back(entry);
        }
    }
    return true;
}

bool WebService::readDisplayOutputs(DisplayConfiguration& configuration) {
    for (size_t index = 0; index < 2; ++index) {
        const String suffix = index == 0 ? "" : "2";
        auto& hardware = configuration.output(index);
        if (server_.hasArg("displayEnabled" + suffix)) {
            if (!parseTextDisplayConfiguration(server_.arg("displayEnabled" + suffix),
                server_.arg("displayBus" + suffix), server_.arg("displayAddress" + suffix), hardware)) return false;
        }
        if (server_.hasArg("displayType" + suffix)
            && !parseTextDisplayType(server_.arg("displayType" + suffix), hardware.type)) return false;
        if (server_.hasArg("displayFont" + suffix)
            && !parseTextDisplayFont(server_.arg("displayFont" + suffix), hardware.font)) return false;
        if (server_.hasArg("displayPage" + suffix)) {
            const String value = server_.arg("displayPage" + suffix);
            if (value != "0" && value != "1") return false;
            configuration.pageAssignment[index] = value == "0" ? 0 : 1;
        }
    }
    return true;
}

void WebService::handleDisplayOutputsSave() {
    for (size_t index = 0; index < 2; ++index) {
        const String suffix = index == 0 ? "" : "2";
        const char* fields[] = {"displayEnabled", "displayType", "displayBus", "displayAddress", "displayPage", "displayFont"};
        for (const char* field : fields) if (!server_.hasArg(String(field) + suffix)) {
            sendResult(uiText(TextId::DisplaySaveFailed), "/display", uiText(TextId::IncompleteDisplaySettingsPreviousSettingsRetained), false); return;
        }
    }
    std::unique_ptr<DisplayConfiguration> storage(new DisplayConfiguration(configurationService_.getConfiguration().display));
    auto& candidate = *storage;
    if (!readDisplayOutputs(candidate)) {
        sendResult(uiText(TextId::DisplaySaveFailed), "/display", uiText(TextId::InvalidDisplayTypeFontBusAddressOrPage), false); return;
    }
    if (candidate.hardware.enabled && candidate.secondHardware.enabled
        && candidate.hardware.bus == candidate.secondHardware.bus
        && candidate.hardware.address == candidate.secondHardware.address) {
        sendResult(uiText(TextId::DisplaySaveFailed), "/display", uiText(TextId::BothEnabledDisplaysUseTheSameI2CBus), false);
        return;
    }
    if (!configurationService_.setDisplayConfiguration(candidate)) {
        sendResult(uiText(TextId::DisplaySaveFailed), "/display", uiText(TextId::CouldNotStoreDisplaySettingsStorageFailureOr), false); return;
    }
    sendSeeOther(server_, "/display");
}

void WebService::handleDisplaySave() {
    if (server_.hasArg("page") && server_.arg("page") != "1" && server_.arg("page") != "2") {
        sendResult(uiText(TextId::DisplaySaveFailed), "/display", uiText(TextId::InvalidPage2), false); return;
    }
    const size_t pageIndex = server_.arg("page") == "2" ? 1 : 0;
    std::unique_ptr<DisplayConfiguration> storage(new DisplayConfiguration(configurationService_.getConfiguration().display));
    auto& candidate = *storage;
    auto& editedPage = candidate.page(pageIndex);
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        const String number(static_cast<unsigned int>(line));
        const String formatField = "f" + number;
        if (!server_.hasArg(formatField)) {
            sendResult(uiText(TextId::DisplaySaveFailed), "/display", uiText(TextId::IncompleteDisplayPagePreviousSettingsRetained), false);
            return;
        }
        editedPage.formats[line] = server_.arg(formatField);
        for (size_t source = 0; source < MaxPropertySourcesPerLine; ++source) {
            const String field = "s" + number + "_" + String(static_cast<unsigned int>(source));
            if (!server_.hasArg(field)) {
                sendResult(uiText(TextId::DisplaySaveFailed), "/display", uiText(TextId::IncompleteDisplayPagePreviousSettingsRetained), false);
                return;
            }
            editedPage.sources[line][source] = server_.arg(field);
            const String suffix = number + "_" + String(static_cast<unsigned int>(source));
            editedPage.labels[line][source].trueText = server_.arg("true" + suffix);
            editedPage.labels[line][source].falseText = server_.arg("false" + suffix);
        }
    }
    ValuePropertyReader valueProperties(valueRuntime_);
    PropertyResolver properties(sensorManager_, measurementSnapshotCache_, actuatorRuntime_, controllerRuntime_, nullptr, &valueProperties);
    if (!readDisplayEnumTranslations(editedPage, properties) || !validateDisplayConfiguration(candidate)) {
        sendResult(uiText(TextId::DisplaySaveFailed), "/display", uiText(TextId::InvalidFormatSourcesOrBooleanTextMatchPlaceholders), false);
        return;
    }
    if (!configurationService_.setDisplayConfiguration(candidate)) {
        sendResult(uiText(TextId::DisplaySaveFailed), "/display", uiText(TextId::CouldNotStoreDisplaySettingsStorageFailureOr), false);
        return;
    }
    sendSeeOther(server_, pageIndex == 0 ? "/display?page=1#text-preview" : "/display?page=2#text-preview");
}

void WebService::handleMeasurements() {
    // Keep previously bookmarked preview URLs usable; ordinary Measurements
    // requests contain no display editor or hardware controls.
    if (server_.hasArg("preview") || server_.hasArg("f0") || server_.hasArg("source")
        || server_.hasArg("format") || server_.hasArg("loadDisplay")) {
        handleDisplay();
        return;
    }
    server_.sendHeader("Cache-Control", "no-store");
    String content;
    content.reserve(600 + sensorManager_.sensorCount() * 1200);
    content = (String("<p class='help'>") + uiText(TextId::CurrentRuntimeSnapshotsOnlyMeasurementsAreNotStored) + "</p>");
    const Configuration& configuration = configurationService_.getConfiguration();
    for (size_t sensorIndex = 0; sensorIndex < sensorManager_.sensorCount(); ++sensorIndex) {
        SensorRuntimeInfo runtime;
        if (!sensorManager_.runtimeInfo(sensorIndex, runtime)) continue;
        content += (String("<section class='card'><h2>") + uiText(TextId::Slot) + " " + "");
        content += localeFormatter_.formatNumber(runtime.id, 0);
        content += " · ";
        content += escapeHtml(runtime.name);
        content += "</h2><p class='help'>";
        content += escapeHtml(runtime.type);
        content += (String("</p><div class='table-scroll measurement-table-wrap'><table class='measurement-table'><thead><tr><th class='measurement-name'>") + uiText(TextId::Measurement) + (String("</th><th class='measurement-value'>") + uiText(TextId::CurrentValue) + "</th><th class='measurement-quality'>") + uiText(TextId::Quality) + (String("</th><th class='measurement-time'>") + uiText(TextId::LastAccepted) + "</th></tr></thead><tbody>"));
        for (uint8_t typeValue = 1; typeValue <= SupportedMeasurementTypeCount; ++typeValue) {
            const MeasurementType type = static_cast<MeasurementType>(typeValue);
            if (!runtimeSupportsMeasurement(runtime, type)) continue;
            MeasurementSnapshot snapshot;
            const bool hasSnapshot = measurementSnapshotCache_.snapshot(runtime.id, type, snapshot);
            content += "<tr><td class='measurement-name'>";
            content += webMeasurementName(configurationService_.getLocale(), type);
            const String mqttTopic = mqttMeasurementTopic(
                configuration.device.name, runtime.id, type);
            content += (String("<span class='secondary'>") + uiText(TextId::MQTT2) + " " + "<code class='measurement-topic'>");
            content += mqttTopic.isEmpty() ? "—" : escapeHtml(mqttTopic);
            content += "</code></span>";
            if (type == MeasurementType::SeaLevelPressure && hasSnapshot) {
                PressureReductionStatus reduction;
                if (sensorManager_.pressureReductionStatus(runtime.id, reduction))
                    content += buildPressureReductionDiagnosticHtml(reduction, configurationService_.getLocale());
            }
            content += "</td><td class='measurement-value'>";
            content += hasSnapshot
                ? presentedMeasurementValue(snapshot.measurement, configuration, localeFormatter_, configurationService_.getLocale())
                : String("—");
            content += "</td><td class='measurement-quality'>";
            content += hasSnapshot ? measurementQualityName(snapshot.measurement.quality, configurationService_.getLocale()) : "—";
            content += "</td><td class='measurement-time'>";
            content += hasSnapshot ? measurementTimeDisplay(snapshot, localeFormatter_, configurationService_.getLocale()) : String("—");
            content += "</td></tr>";
        }
        content += "</tbody></table></div></section>";
    }
    if (sensorManager_.sensorCount() == 0) {
        content += (String("<section class='card'><p>") + uiText(TextId::NoActiveRuntimeSensors) + "</p></section>");
    }
    sendPage(uiText(TextId::Measurements), "/measurements", content);
}

void WebService::handleDisplay() {
    server_.sendHeader("Cache-Control", "no-store");
    String content;
    const Configuration& configuration = configurationService_.getConfiguration();
    if (!server_.hasArg("page") && !server_.hasArg("f0") && !server_.hasArg("source")
        && !server_.hasArg("format") && !server_.hasArg("preview") && !server_.hasArg("loadDisplay")) {
        sendPage(uiText(TextId::Display), "/display", buildDisplayOutputsHtml(configuration.display, configurationService_.getLocale()));
        return;
    }

    SystemPropertyReader timeProperties(timeService_, localeFormatter_, FirmwareBuildInfo::SemanticVersion, FirmwareBuildInfo::BuildNumber, FirmwareBuildInfo::GitCommit, FirmwareBuildInfo::GitBranch, FirmwareBuildInfo::CompactIdentity, &configurationService_, &wifiService_);
    ValuePropertyReader valueProperties(valueRuntime_);
    PropertyResolver canonicalProperties(sensorManager_, measurementSnapshotCache_, actuatorRuntime_, controllerRuntime_, &timeProperties, &valueProperties);
    PropertyPresentationReader properties(canonicalProperties, configuration.presentation);
    const bool loadSavedDisplay = server_.hasArg("loadDisplay");
    const bool previewSubmitted = !loadSavedDisplay && server_.hasArg("preview");
    if (server_.hasArg("page") && server_.arg("page") != "1" && server_.arg("page") != "2") {
        sendResult(uiText(TextId::InvalidPage), "/display", uiText(TextId::ChoosePage1OrPage2), false); return;
    }
    const size_t pageIndex = server_.arg("page") == "2" ? 1 : 0;
    std::unique_ptr<DisplayConfiguration> storage(new DisplayConfiguration(configuration.display));
    auto& previewPage = *storage;
    auto& editedPage = previewPage.page(pageIndex);
    const bool multiLineInput = !loadSavedDisplay && server_.hasArg("f0");
    if (multiLineInput) for (size_t line = 0; line < PropertyPreviewLineCount; ++line) {
        const String number(static_cast<unsigned int>(line));
        editedPage.formats[line] = server_.arg("f" + number);
        for (size_t source = 0; source < MaxPropertySourcesPerLine; ++source) {
            const String suffix = number + "_" + String(static_cast<unsigned int>(source));
            editedPage.sources[line][source] = server_.arg("s" + suffix);
            editedPage.labels[line][source].trueText = server_.arg("true" + suffix);
            editedPage.labels[line][source].falseText = server_.arg("false" + suffix);
        }
    }
    if (multiLineInput && !readDisplayEnumTranslations(editedPage, properties)) {
        sendResult(uiText(TextId::InvalidStateTranslations), "/display", uiText(TextId::UseAtMost32StateTranslationsEachUp), false);
        return;
    }
    // Existing single-line bookmarks become line 1.
    if (!loadSavedDisplay && !multiLineInput && (server_.hasArg("source") || server_.hasArg("format"))) {
        editedPage = DisplayPage{};
        editedPage.sources[0][0] = server_.arg("source");
        editedPage.formats[0] = server_.arg("format");
    }
    String previewOptions;
    const auto addPreviewSource = [&](const PropertyReference& reference, const char* name) {
        PropertyDescription description;
        if (!properties.describe(reference, description)) return;
        const String source = propertySourceText(reference);
        if (!loadSavedDisplay && !configuration.display.configured && !previewSubmitted && !multiLineInput && !server_.hasArg("source") && editedPage.sources[0][0].isEmpty()) {
            editedPage.sources[0][0] = source;
            if (!server_.hasArg("format")) {
                editedPage.formats[0] = description.valueKind == PropertyValueKind::FloatingPoint ? "%.1f"
                    : description.valueKind == PropertyValueKind::UnsignedInteger ? "%u" : "%s";
            }
        }
        // One shared source inventory avoids duplicating it 24 times in the response.
        previewOptions += buildPropertySourceOption(reference, description, name, String(), configurationService_.getLocale());
    };
    for (size_t index = 0; index < sensorManager_.sensorCount(); ++index) {
        SensorRuntimeInfo runtime;
        if (!sensorManager_.runtimeInfo(index, runtime)) continue;
        for (uint8_t value = 1; value <= SupportedMeasurementTypeCount; ++value) {
            addPreviewSource(PropertyReference(PropertyComponentKind::Sensor, runtime.id,
                measurementTypeStableId(static_cast<MeasurementType>(value))), runtime.name);
        }
    }
    for (size_t index = 0; index < actuatorRuntime_.runtimeCount(); ++index) {
        ActuatorRuntimeInfo runtime;
        if (actuatorRuntime_.runtimeInfo(index, runtime)) {
            addPreviewSource(PropertyReference(PropertyComponentKind::Actuator, runtime.id, "state"), runtime.name);
            addPreviewSource(PropertyReference(PropertyComponentKind::Actuator, runtime.id, "level"), runtime.name);
        }
    }
    for (size_t index = 0; index < controllerRuntime_.runtimeCount(); ++index) {
        ControllerRuntimeInfo runtime;
        if (controllerRuntime_.runtimeInfo(index, runtime)) {
            addPreviewSource(PropertyReference(PropertyComponentKind::Controller, runtime.id, "decision"), runtime.name);
            addPreviewSource(PropertyReference(PropertyComponentKind::Controller, runtime.id, "reason"), runtime.name);
        }
    }
    for (const auto& value : valueRuntime_.values()) {
        addPreviewSource(PropertyReference(PropertyComponentKind::Value, value.configuration().id, "state"), value.configuration().name.c_str());
    }
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "device_name"), uiText(TextId::DeviceName));
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "hostname"), uiText(TextId::Hostname));
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "ip_address"), uiText(TextId::IPv4Address));
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "version"), uiText(TextId::FirmwareVersion2));
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "build"), uiText(TextId::BuildNumber2));
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "git_commit"), uiText(TextId::GitCommit2));
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "git_branch"), uiText(TextId::GitBranch));
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "build_identity"), uiText(TextId::FullBuildIdentity));
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "date"), uiText(TextId::DateLocale));
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "time"), uiText(TextId::TimeLocale));
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "datetime"), uiText(TextId::DateAndTimeLocale));
    int previewOutput = -1;
    if (server_.hasArg("previewOutput")) {
        const String value = server_.arg("previewOutput");
        if (value == "full") previewOutput = -2;
        else if (value == "0" || value == "1") {
            previewOutput = value == "0" ? 0 : 1;
            if (previewPage.pageAssignment[previewOutput] != pageIndex) previewOutput = -2;
        } else { sendResult(uiText(TextId::InvalidPreview), "/display", uiText(TextId::ChooseADisplayOrFullPage), false); return; }
    }
    // Move the result instead of allocating a second complete copy with +=.
    content = buildPropertyPagePreviewHtml(properties, previewOptions, previewPage, loadSavedDisplay || previewSubmitted || configuration.display.configured, pageIndex, previewOutput, configurationService_.getLocale());
    // These temporary inputs are no longer needed while sending the page header/body.
    previewOptions = String();
    storage.reset();
    sendPage(uiText(TextId::Display), "/display", content);
}

void WebService::handleSensorEdit() {
    const long requestedSlot = server_.arg("slot").toInt();
    if (requestedSlot < 1 || requestedSlot > static_cast<long>(MaxSensorSlotCount)) {
        sendResult(uiText(TextId::InvalidSensorSlot), "/sensors", uiText(TextId::TheRequestedSlotDoesNotExist), false);
        return;
    }
    const SensorSlotConfiguration& slot =
        configurationService_.getConfiguration().sensorSlots[requestedSlot - 1];
    const SensorImplementationMetadata* selected = SensorImplementationRegistry::find(slot.implementation);
    String options;
    for (size_t index = 0; index < SensorImplementationRegistry::count(); ++index) {
        const SensorImplementationMetadata* metadata = SensorImplementationRegistry::at(index);
        if (metadata == nullptr) continue;
        options += "<option value='" + String(metadata->stableId) + "' data-interface='";
        options += hardwareInterfaceKindName(metadata->interfaceKind);
        options += "' data-kind='" + String(metadata->stableId)
            + "' data-requires='"
            + String(static_cast<unsigned>(metadata->requiredGpioCapabilities))
            + "' data-interval='" + String(metadata->defaultSchedule.sampleIntervalMs) + "'";
        if (metadata->implementation == slot.implementation) options += " selected";
        options += ">" + escapeHtml(webSensorImplementationName(configurationService_.getLocale(), metadata->implementation)) + "</option>";
    }
    String gpioOptions;
    String i2cBusOptions;
    const BoardCapabilities& board = BoardCapabilities::current();
    const Configuration& configuration = configurationService_.getConfiguration();
    for (size_t index = 0; index < board.gpioCount(); ++index) {
        const BoardGpioCapability* gpio = board.gpioAt(index);
        if (gpio == nullptr) continue;
        if (gpioAssignedToOtherEnabledSlot(
                configuration, slot.slotId, InvalidActuatorId, gpio->resource)) {
            continue;
        }
        gpioOptions += "<option value='" + String(gpio->resource.number)
            + "' data-capabilities='"
            + String(static_cast<unsigned>(gpio->capabilities)) + "'";
        if (slot.hardware.kind == HardwareResourceKind::GPIO
            && slot.hardware.gpio.number == gpio->resource.number) gpioOptions += " selected";
        gpioOptions += ">" + String(gpio->displayName) + "</option>";
    }
    for (size_t index = 0; index < board.i2cBusCount(); ++index) {
        const BoardI2CBusCapability* bus = board.i2cBusAt(index);
        if (bus == nullptr) continue;
        i2cBusOptions += "<option value='" + String(static_cast<unsigned>(bus->bus)) + "'";
        if (slot.hardware.kind == HardwareResourceKind::I2C
            && slot.hardware.i2c.bus == bus->bus) i2cBusOptions += " selected";
        i2cBusOptions += ">" + String(i2cBusName(bus->bus)) + "</option>";
    }
    String c;
    c.reserve(2600);
    c = (String("<section class='card'><h2>") + uiText(TextId::ConfigureSlot) + " " + "") + String(slot.slotId) + "</h2><form method='post' action='/sensors/save'><input type='hidden' name='slot' value='" + String(slot.slotId) + "'>";
    c += "<label class='choice'><input type='checkbox' name='enabled' value='1'" + String(slot.enabled ? " checked" : "") + (String(">") + uiText(TextId::Enabled) + "</label>");
    c += (String("<label>") + uiText(TextId::Name) + "<input name='name' maxlength='") + String(MaxSensorSlotNameLength) + "' required value='" + escapeHtml(slot.name) + "'></label>";
    c += (String("<label>") + uiText(TextId::Implementation) + "<select id='sensorImplementation' name='implementation'>") + options + "</select></label>";
    c += "<div id='gpioConfiguration'><label>GPIO<select name='gpio'>" + gpioOptions + (String("</select></label><p class='help'>") + uiText(TextId::AM2302UsesACustomSingleWireProtocolRain) + "</p></div>");
    c += (String("<div id='i2cConfiguration'><label>") + uiText(TextId::Bus) + "<select name='i2cBus'>") + i2cBusOptions + (String("</select></label><label>") + uiText(TextId::ICAddress) + "<select id='i2cAddress' name='i2cAddress'><option value='68'") + String(slot.hardware.kind == HardwareResourceKind::I2C && slot.hardware.i2c.address == 0x44 ? " selected" : "") + ">0x44</option><option value='112'" + String(slot.hardware.kind == HardwareResourceKind::I2C && slot.hardware.i2c.address == 0x70 ? " selected" : "") + ">0x70</option><option value='118'" + String(slot.hardware.kind == HardwareResourceKind::I2C && slot.hardware.i2c.address == 0x76 ? " selected" : "") + ">0x76</option><option value='119'" + String(slot.hardware.kind == HardwareResourceKind::I2C && slot.hardware.i2c.address == 0x77 ? " selected" : "") + ">0x77</option></select></label></div>";
    c += (String("<div id='rainGaugeConfiguration'><label>") + uiText(TextId::MillimetresPerTip) + "<input type='number' name='millimetersPerTip' min='0.0001' max='100' step='0.0001' value='") + String(slot.implementationConfiguration.rainGauge.millimetersPerTip, 4) + (String("'></label><label>") + uiText(TextId::DebounceTimeMs) + "<input type='number' name='debounceMs' min='1' max='5000' value='") + String(slot.implementationConfiguration.rainGauge.debounceMs) + "'></label></div>";
    c += (String("<label>") + uiText(TextId::SampleIntervalMs) + "<input id='sensorInterval' type='number' min='1' max='2147483647' name='interval' value='") + String(slot.schedule.sampleIntervalMs) + "'></label>";
    c += (String("<div class='actions'><button type='submit'>") + uiText(TextId::SaveSlot) + "</button><a class='button' href='/sensors'>" + uiText(TextId::Cancel) + "</a></div></form></section>");
    if (selected != nullptr) {
        c += (((String("<section class='card'><h2>") + uiText(TextId::ImplementationMetadata) + "</h2><div class='kv'><span>")) + uiText(TextId::Type) + "</span><span>") + escapeHtml(webSensorImplementationName(configurationService_.getLocale(), selected->implementation)) + (String("</span><span>") + uiText(TextId::Interface) + "</span><span>") + hardwareInterfaceKindName(selected->interfaceKind) + " / " + escapeHtml(webSensorDescription(configurationService_.getLocale(), selected->implementation)) + (String("</span><span>") + uiText(TextId::Provenance) + "</span><span>") + String(selected->provenance == SensorProvenance::Simulated ? uiText(TextId::Simulated) : uiText(TextId::Physical)) + (String("</span><span>") + uiText(TextId::Measurements) + "</span><span>") + implementationMeasurements(selected, configurationService_.getLocale()) + "</span></div></section>";
    }
    c += "<script>function sensorFields(reset){const s=document.getElementById('sensorImplementation');const o=s.options[s.selectedIndex];document.getElementById('gpioConfiguration').style.display=o.dataset.interface==='GPIO'?'block':'none';document.getElementById('i2cConfiguration').style.display=o.dataset.interface==='I2C'?'block':'none';document.getElementById('rainGaugeConfiguration').style.display=o.dataset.kind==='rain_gauge'?'block':'none';const g=document.querySelector('[name=gpio]');const r=Number(o.dataset.requires);for(const x of g.options)x.hidden=(Number(x.dataset.capabilities)&r)!==r;if(reset&&o.dataset.interface==='GPIO'&&(g.selectedOptions.length===0||g.selectedOptions[0].hidden)){const x=Array.from(g.options).find(x=>!x.hidden);if(x)g.value=x.value}const a=document.getElementById('i2cAddress');for(const x of a.options)x.hidden=o.dataset.kind==='sht4x'?x.value!=='68':o.dataset.kind==='shtc3'?x.value!=='112':o.dataset.kind==='bme280'?(x.value!=='118'&&x.value!=='119'):false;if(reset&&o.dataset.kind==='sht4x')a.value='68';if(reset&&o.dataset.kind==='shtc3')a.value='112';if(reset&&o.dataset.kind==='bme280'&&(a.value==='68'||a.value==='112'))a.value='118';const n=Number(o.dataset.interval);const f=document.getElementById('sensorInterval');f.parentElement.style.display=n>0?'block':'none';f.disabled=n<=0;if(reset)f.value=n}document.getElementById('sensorImplementation').addEventListener('change',()=>sensorFields(true));sensorFields(false);</script>";
    sendPage(uiText(TextId::ConfigureSensorSlot), "/sensors", c);
}

void WebService::handleActuatorEdit() {
    const long requestedSlot = server_.arg("slot").toInt();
    if (requestedSlot < 1 || requestedSlot > static_cast<long>(MaxActuatorSlotCount)) {
        sendResult(uiText(TextId::InvalidActuatorSlot), "/actuators",
            uiText(TextId::TheRequestedSlotDoesNotExist), false);
        return;
    }
    const ActuatorSlotConfiguration& slot =
        configurationService_.getConfiguration().actuatorSlots[requestedSlot - 1];
    ActuatorRuntimeInfo runtime;
    const bool hasRuntime = actuatorInfoById(actuatorRuntime_, slot.slotId, runtime);
    ModuleActuatorReference reference = slot.moduleTarget;
    const bool savedModule = validModuleActuatorReference(reference);
    const bool module = savedModule || actuatorRuntime_.moduleReference(slot.slotId, reference);
    if (module || (hasRuntime && runtime.origin == ActuatorRuntimeOrigin::ModuleDescriptor)) {
        String c = (String("<section class='card'><h2>") + uiText(TextId::ConfigureModuleOnOffActuator) + "</h2>");
        c += "<p>" + escapeHtml(moduleActuatorLabel(moduleDiscoveryService_, reference, configurationService_.getLocale())) + "</p>";
        if (hasRuntime) {
            c += (String("<p>") + uiText(TextId::Module) + " " + "") + escapeHtml(moduleSlotName(runtime.moduleSlot)) + " / "
                + escapeHtml(runtime.descriptorDeviceId) + " · "
                + configuredHardwareAssignment(runtime.hardware, configurationService_.getLocale()) + "</p>";
        } else {
            c += (String("<p>") + uiText(TextId::ModuleActuatorIsDisabledOrCurrentlyUnavailableIts) + "</p>");
        }
        c += (String("<p class='help'>") + uiText(TextId::OnOffCapabilityAndHardwareAreDefinedBy) + "</p>");
        if (module) {
            c += "<form method='post' action='/actuators/save'><input type='hidden' name='slot' value='"
                + String(slot.slotId) + "'><input type='hidden' name='module' value='"
                + moduleActuatorToken(reference) + "'>";
            c += "<label class='choice'><input type='checkbox' name='enabled' value='1'"
                + String((savedModule ? slot.enabled : true) ? " checked" : "") + (String(">") + uiText(TextId::Enabled) + "</label>");
            c += (String("<label>") + uiText(TextId::Name) + "<input name='name' required maxlength='") + String(MaxActuatorSlotNameLength)
                + "' value='" + escapeHtml(savedModule ? slot.name : String(runtime.name)) + "'></label>";
            c += (String("<div class='actions'><button type='submit'>") + uiText(TextId::SaveModuleActuator) + "</button><a class='button' href='/actuators'>" + uiText(TextId::Cancel) + "</a></div></form>");
        } else {
            c += (String("<p>") + uiText(TextId::AModuleInstanceIdentityIsRequiredToSave) + "</p>");
        }
        c += "</section>";
        sendPage(uiText(TextId::ConfigureModuleActuator), "/actuators", c);
        return;
    }
    const ActuatorImplementationMetadata* selected =
        ActuatorImplementationRegistry::find(slot.implementation);
    String options;
    for (size_t index = 0; index < ActuatorImplementationRegistry::count(); ++index) {
        const ActuatorImplementationMetadata* metadata =
            ActuatorImplementationRegistry::at(index);
        if (metadata == nullptr) continue;
        options += "<option value='" + String(metadata->stableId)
            + "' data-interface='" + hardwareInterfaceKindName(metadata->interfaceKind)
            + "' data-requires='"
            + String(static_cast<unsigned>(metadata->requiredGpioCapabilities)) + "'";
        if (metadata->implementation == slot.implementation) options += " selected";
        options += ">" + escapeHtml(webActuatorImplementationName(configurationService_.getLocale(), metadata->implementation)) + "</option>";
    }
    String gpioOptions;
    const BoardCapabilities& board = BoardCapabilities::current();
    const Configuration& configuration = configurationService_.getConfiguration();
    for (size_t index = 0; index < board.gpioCount(); ++index) {
        const BoardGpioCapability* gpio = board.gpioAt(index);
        if (gpio == nullptr
            || actuatorRuntime_.moduleOwnsHardware(HardwareResourceAssignment::gpioResource(gpio->resource))
            || gpioAssignedToOtherEnabledSlot(
                configuration, InvalidSensorId, slot.slotId, gpio->resource)) {
            continue;
        }
        gpioOptions += "<option value='" + String(gpio->resource.number)
            + "' data-capabilities='"
            + String(static_cast<unsigned>(gpio->capabilities)) + "'";
        if (slot.hardware.kind == HardwareResourceKind::GPIO
            && slot.hardware.gpio.number == gpio->resource.number) {
            gpioOptions += " selected";
        }
        gpioOptions += ">" + String(gpio->displayName) + "</option>";
    }
    String c;
    c.reserve(2200);
    c = (String("<section class='card'><h2>") + uiText(TextId::ConfigureSlot) + " " + "") + String(slot.slotId)
        + (String("</h2><p class='help'>") + uiText(TextId::SavedChangesBecomeActiveAfterApplyingTheActuator) + "</p><form method='post' action='/actuators/save'><input type='hidden' name='slot' value='")
        + String(slot.slotId) + "'>";
    c += "<label class='choice'><input type='checkbox' name='enabled' value='1'"
        + String(slot.enabled ? " checked" : "") + (String(">") + uiText(TextId::Enabled) + "</label>");
    c += (String("<label>") + uiText(TextId::Name) + "<input name='name' maxlength='")
        + String(MaxActuatorSlotNameLength) + "' required value='"
        + escapeHtml(slot.name) + "'></label>";
    c += (String("<label>") + uiText(TextId::Implementation) + "<select id='actuatorImplementation' name='implementation'>")
        + options + "</select></label>";
    c += "<div id='actuatorGpioConfiguration'><label>GPIO<select name='gpio'>"
        + gpioOptions + "</select></label></div>";
    c += (String("<div class='actions'><button type='submit'>") + uiText(TextId::SaveSlot) + "</button><a class='button' href='/actuators'>" + uiText(TextId::Cancel) + "</a></div></form></section>");
    if (selected != nullptr) {
        c += (((String("<section class='card'><h2>") + uiText(TextId::ImplementationMetadata) + "</h2><div class='kv'><span>")) + uiText(TextId::Type) + "</span><span>")
            + escapeHtml(webActuatorImplementationName(configurationService_.getLocale(), selected->implementation)) + (String("</span><span>") + uiText(TextId::Interface) + "</span><span>")
            + hardwareInterfaceKindName(selected->interfaceKind) + " / "
            + escapeHtml(webActuatorDescription(configurationService_.getLocale(), selected->implementation))
            + (String("</span><span>") + uiText(TextId::Capability) + "</span><span>")
            + String(hasActuatorCapability(selected->capabilities, ActuatorCapability::Level)
                ? uiText(TextId::OnOffLevel)
                : hasActuatorCapability(selected->capabilities, ActuatorCapability::OnOff)
                    ? (uiText(TextId::OnOff)) : uiText(TextId::None)) + "</span></div></section>";
    }
    c += "<script>function actuatorFields(reset){const s=document.getElementById('actuatorImplementation');const o=s.options[s.selectedIndex];const p=document.getElementById('actuatorGpioConfiguration');p.style.display=o.dataset.interface==='GPIO'?'block':'none';const g=document.querySelector('[name=gpio]');const r=Number(o.dataset.requires);for(const x of g.options)x.hidden=(Number(x.dataset.capabilities)&r)!==r;if(reset&&o.dataset.interface==='GPIO'&&(g.selectedOptions.length===0||g.selectedOptions[0].hidden)){const x=Array.from(g.options).find(x=>!x.hidden);if(x)g.value=x.value}}document.getElementById('actuatorImplementation').addEventListener('change',()=>actuatorFields(true));actuatorFields(false);</script>";
    sendPage(uiText(TextId::ConfigureActuatorSlot), "/actuators", c);
}

void WebService::handleControllerEdit() {
    const long requestedSlot = server_.arg("slot").toInt();
    if (requestedSlot < 1 || requestedSlot > static_cast<long>(MaxControllerSlotCount)) {
        sendResult(uiText(TextId::InvalidControllerSlot), "/controllers",
            uiText(TextId::TheRequestedSlotDoesNotExist), false);
        return;
    }
    const Configuration& configuration = configurationService_.getConfiguration();
    const ControllerSlotConfiguration& slot =
        configuration.controllerSlots[requestedSlot - 1];
    const ControllerImplementationMetadata* selected =
        ControllerImplementationRegistry::find(slot.implementation);
    String implementationOptions;
    for (size_t index = 0; index < ControllerImplementationRegistry::count(); ++index) {
        const ControllerImplementationMetadata* metadata =
            ControllerImplementationRegistry::at(index);
        if (metadata == nullptr) continue;
        implementationOptions += "<option value='" + String(metadata->stableId)
            + "' data-kind='" + String(metadata->stableId) + "'";
        if (metadata->implementation == slot.implementation) {
            implementationOptions += " selected";
        }
        implementationOptions += ">" + escapeHtml(webControllerImplementationName(configurationService_.getLocale(), metadata->implementation)) + "</option>";
    }
    String blinkTargetOptions;
    String thresholdTargetOptions;
    bool moduleTargetListed = false;
    const auto targetClaimed = [&](ActuatorId id, const ModuleActuatorReference* reference) {
        ControllerSlotConfiguration target;
        target.implementation = ControllerImplementation::Blink;
        target.implementationConfiguration.blink.targetActuatorId = id;
        if (reference) target.moduleTarget = *reference;
        for (const auto& other : configuration.controllerSlots) {
            if (other.enabled && other.slotId != slot.slotId && controllerTargetsSameActuator(
                target, other, configuration.actuatorSlots, &actuatorRuntime_)) return true;
        }
        return false;
    };
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        const ActuatorSlotConfiguration& actuator = configuration.actuatorSlots[index];
        ModuleActuatorReference moduleAlias;
        // This slot is represented once below, with its module identity and user name.
        if (actuatorRuntime_.moduleReference(actuator.slotId, moduleAlias)
            || targetClaimed(actuator.slotId, nullptr)) continue;
        const ActuatorImplementationMetadata* metadata =
            ActuatorImplementationRegistry::find(actuator.implementation);
        if (metadata == nullptr
            || !hasActuatorCapability(metadata->capabilities, ActuatorCapability::OnOff)) {
            continue;
        }
        blinkTargetOptions += "<option value='" + String(actuator.slotId) + "'";
        if (slot.implementationConfiguration.blink.targetActuatorId == actuator.slotId) {
            blinkTargetOptions += " selected";
        }
        blinkTargetOptions += (String(">") + uiText(TextId::Actuator) + " " + "") + String(actuator.slotId) + " · "
            + escapeHtml(actuator.name);
        if (!actuator.enabled) blinkTargetOptions += (String(" ") + uiText(TextId::Disabled2));
        blinkTargetOptions += "</option>";

        if (isEligibleThresholdActuator(actuator)) {
            thresholdTargetOptions += "<option value='" + String(actuator.slotId) + "'";
            if (slot.implementationConfiguration.threshold.targetActuatorId
                == actuator.slotId) {
                thresholdTargetOptions += " selected";
            }
            thresholdTargetOptions += (String(">") + uiText(TextId::ActuatorSlot) + " " + "") + String(actuator.slotId)
                + " · " + escapeHtml(actuator.name) + "</option>";
        }
    }
    for (size_t runtimeIndex = 0; runtimeIndex < actuatorRuntime_.runtimeCount();
         ++runtimeIndex) {
        ActuatorRuntimeInfo runtime;
        ModuleActuatorReference reference;
        if (!actuatorRuntime_.runtimeInfo(runtimeIndex, runtime)
            || runtime.origin != ActuatorRuntimeOrigin::ModuleDescriptor
            || !actuatorRuntime_.moduleReference(runtime.id, reference)
            || !hasActuatorCapability(runtime.capabilities, ActuatorCapability::OnOff)) {
            continue;
        }
        if (targetClaimed(runtime.id, &reference)) continue;
        const auto& configured = configuration.actuatorSlots[runtime.id - 1];
        const bool namedSlot = configured.implementation != ActuatorImplementation::None;
        ActuatorId savedTarget = InvalidActuatorId;
        configuredControllerTargetActuatorId(slot, savedTarget);
        const bool savedNumeric = !validModuleActuatorReference(slot.moduleTarget) && savedTarget == runtime.id;
        // Preserve an existing numeric reference; new choices use stable module identity.
        const String value = savedNumeric ? String(runtime.id) : "m:" + String(runtime.id);
        const String label = (namedSlot ? configured.name : String(runtime.name))
            + (String(" · ") + uiText(TextId::Module) + " ") + String(moduleSlotName(runtime.moduleSlot)) + " · " + String(runtime.descriptorDeviceId);
        blinkTargetOptions += "<option value='" + value + "'";
        if (sameModuleActuatorReference(
                slot.moduleTarget, reference) || savedNumeric) {
            blinkTargetOptions += " selected";
            moduleTargetListed = true;
        }
        blinkTargetOptions += ">" + escapeHtml(label) + "</option>";
        thresholdTargetOptions += "<option value='" + value + "'";
        if (sameModuleActuatorReference(
                slot.moduleTarget, reference) || savedNumeric) {
            thresholdTargetOptions += " selected";
            moduleTargetListed = true;
        }
        thresholdTargetOptions += ">" + escapeHtml(label) + "</option>";
    }
    if (validModuleActuatorReference(slot.moduleTarget) && !moduleTargetListed) {
        const String unavailable =
            (String("<option value='m:current' selected>") + uiText(TextId::ConfiguredModuleActuatorCurrentlyUnavailable) + "</option>");
        blinkTargetOptions += unavailable;
        thresholdTargetOptions += unavailable;
    }
    if (blinkTargetOptions.isEmpty()) {
        blinkTargetOptions = (String("<option value='0'>") + uiText(TextId::NoCompatibleOnOffActuatorConfigured) + "</option>");
    }
    if (thresholdTargetOptions.isEmpty()) {
        thresholdTargetOptions = (String("<option value='0'>") + uiText(TextId::NoEnabledOnOffActuatorConfigured) + "</option>");
    }

    const BlinkControllerConfiguration& blink = slot.implementationConfiguration.blink;
    const ThresholdControllerConfiguration& threshold =
        slot.implementationConfiguration.threshold;
    String sourceOptions = (String("<option value='0'>") + uiText(TextId::SelectACompatibleSensor) + "</option>");
    String measurementOptions =
        (String("<option value='unknown' data-unit=''>") + uiText(TextId::SelectAMeasurement) + "</option>");
    String measurementCatalog = "{";
    bool firstCatalogSensor = true;
    for (size_t sensorIndex = 0; sensorIndex < MaxSensorSlotCount; ++sensorIndex) {
        const SensorSlotConfiguration& sensor = configuration.sensorSlots[sensorIndex];
        if (!isEligibleThresholdSensor(sensor)) continue;
        sourceOptions += "<option value='" + String(sensor.slotId) + "'";
        if (threshold.source.sensorId == sensor.slotId) sourceOptions += " selected";
        sourceOptions += (String(">") + uiText(TextId::SensorSlot) + " " + "") + String(sensor.slotId) + " · "
            + escapeHtml(sensor.name) + "</option>";
        MeasurementType types[MaxImplementationMeasurementTypeCount];
        const size_t typeCount = thresholdMeasurementTypesForSensor(
            sensor, types, MaxImplementationMeasurementTypeCount);
        if (!firstCatalogSensor) measurementCatalog += ",";
        firstCatalogSensor = false;
        measurementCatalog += "'" + String(sensor.slotId) + "':[";
        for (size_t typeIndex = 0; typeIndex < typeCount; ++typeIndex) {
            const MeasurementType type = types[typeIndex];
            const MeasurementTypeMetadata& typeMetadata = measurementTypeMetadata(type);
            const char* unit = UnitConverter::symbol(typeMetadata.canonicalUnit);
            if (typeIndex != 0) measurementCatalog += ",";
            measurementCatalog += "{value:'" + String(typeMetadata.stableId)
                + "',label:'" + escapeHtml(webMeasurementName(configurationService_.getLocale(), typeMetadata.type))
                + "',unit:'" + escapeHtml(unit == nullptr ? "" : unit) + "'}";
            if (threshold.source.sensorId == sensor.slotId) {
                measurementOptions += "<option value='" + String(typeMetadata.stableId)
                    + "' data-unit='" + escapeHtml(unit == nullptr ? "" : unit) + "'";
                if (threshold.source.measurementType == type) {
                    measurementOptions += " selected";
                }
                measurementOptions += ">" + escapeHtml(webMeasurementName(configurationService_.getLocale(), typeMetadata.type))
                    + "</option>";
            }
        }
        measurementCatalog += "]";
    }
    measurementCatalog += "}";
    String c;
    c.reserve(4200 + sourceOptions.length() + measurementOptions.length());
    c = (String("<section class='card'><h2>") + uiText(TextId::ConfigureSlot) + " " + "") + String(slot.slotId)
        + (String("</h2><p class='help'>") + uiText(TextId::SavedChangesBecomeActiveAfterApplyingTheController) + "</p><form method='post' action='/controllers/save'><input type='hidden' name='slot' value='")
        + String(slot.slotId) + "'>";
    c += "<label class='choice'><input type='checkbox' name='enabled' value='1'"
        + String(slot.enabled ? " checked" : "") + (String(">") + uiText(TextId::Enabled) + "</label>");
    c += (String("<label>") + uiText(TextId::Name) + "<input name='name' maxlength='") + String(MaxControllerSlotNameLength)
        + "' required value='" + escapeHtml(slot.name) + "'></label>";
    c += (String("<label>") + uiText(TextId::Implementation) + "<select id='controllerImplementation' name='implementation'>")
        + implementationOptions + "</select></label>";
    c += (String("<div id='blinkConfiguration'><label>") + uiText(TextId::TargetOnOffActuator) + "<select name='targetActuator'>")
        + blinkTargetOptions + "</select></label>";
    c += (String("<label>") + uiText(TextId::OnDurationMs) + "<input type='number' name='onDuration' min='1' max='2147483647' value='")
        + String(blink.onDurationMs) + "'></label>";
    c += (String("<label>") + uiText(TextId::OffDurationMs) + "<input type='number' name='offDuration' min='1' max='2147483647' value='")
        + String(blink.offDurationMs) + "'></label></div>";
    c += (String("<div id='thresholdConfiguration'><label>") + uiText(TextId::SourceSensor) + "<select id='thresholdSourceSensor' name='thresholdSourceSensor'>")
        + sourceOptions + "</select></label>";
    c += (String("<label>") + uiText(TextId::Measurement) + "<select id='thresholdMeasurement' name='thresholdMeasurement'>")
        + measurementOptions + "</select></label>";
    c += (String("<p class='help'>") + uiText(TextId::ThresholdValuesUseTheCanonicalMeasurementUnit) + " " + "<span id='thresholdUnit'></span>.</p>");
    c += "<label class='choice'><input id='thresholdDecisionOnly' type='checkbox' name='decisionOnly' value='1'"
        + String(threshold.decisionOnly ? " checked" : "") + (String(">") + uiText(TextId::DecisionOnlyNoActuatorControl) + "</label>");
    c += (String("<label id='thresholdTargetField'>") + uiText(TextId::TargetOnOffActuator) + "<select name='thresholdTargetActuator'>")
        + thresholdTargetOptions + "</select></label>";
    c += (String("<label>") + uiText(TextId::SwitchingDirection) + "<select name='thresholdDirection'><option value='on_above'")
        + String(threshold.direction == ThresholdDirection::OnAbove ? " selected" : "")
        + (String(">") + uiText(TextId::OnAtOrAboveTheOnThreshold) + "</option><option value='on_below'")
        + String(threshold.direction == ThresholdDirection::OnBelow ? " selected" : "")
        + (String(">") + uiText(TextId::OnAtOrBelowTheOnThreshold) + "</option></select></label>");
    c += (String("<label>") + uiText(TextId::OnThreshold) + "<input type='number' step='any' name='onThreshold' value='")
        + String(threshold.onThreshold, 6) + "'></label>";
    c += (String("<label>") + uiText(TextId::OffThreshold) + "<input type='number' step='any' name='offThreshold' value='")
        + String(threshold.offThreshold, 6) + "'></label>";
    c += (String("<label>") + uiText(TextId::MaximumMeasurementAgeMs) + "<input type='number' min='1' max='2147483647' name='maxMeasurementAge' value='")
        + String(threshold.maxMeasurementAgeMs) + "'></label></div>";
    String selectorTargets = thresholdTargetOptions;
    selectorTargets.replace(" selected", "");
    const String selectorTarget = validModuleActuatorReference(slot.moduleTarget)
        ? String("m:current") : String(slot.implementationConfiguration.selector.targetActuatorId);
    selectorTargets.replace("value='" + selectorTarget + "'", "value='" + selectorTarget + "' selected");
    // Listed module targets use their current runtime ID.
    if (validModuleActuatorReference(slot.moduleTarget)) {
        for (size_t i = 0; i < actuatorRuntime_.runtimeCount(); ++i) {
            ActuatorRuntimeInfo info; ModuleActuatorReference ref;
            if (actuatorRuntime_.runtimeInfo(i, info) && actuatorRuntime_.moduleReference(info.id, ref)
                && sameModuleActuatorReference(ref, slot.moduleTarget)) {
                const String token = "value='m:" + String(info.id) + "'";
                selectorTargets.replace(token, token + " selected");
            }
        }
    }
    c += buildSelectorFields(configuration, slot, selectorTargets);
    c += (String("<div class='actions'><button type='submit'>") + uiText(TextId::SaveSlot) + "</button><a class='button' href='/controllers'>" + uiText(TextId::Cancel) + "</a></div></form></section>");
    if (selected != nullptr) {
        c += (((String("<section class='card'><h2>") + uiText(TextId::ImplementationMetadata) + "</h2><div class='kv'><span>")) + uiText(TextId::Type) + "</span><span>")
            + escapeHtml(webControllerImplementationName(configurationService_.getLocale(), selected->implementation)) + (String("</span><span>") + uiText(TextId::RequiredActuatorCapability) + "</span><span>")
            + String(slot.implementation == ControllerImplementation::Threshold && threshold.decisionOnly
                ? uiText(TextId::None) : hasActuatorCapability(selected->requiredActuatorCapabilities,
                ActuatorCapability::OnOff) ? (uiText(TextId::OnOff)) : uiText(TextId::None))
            + (String("</span><span>") + uiText(TextId::Description) + "</span><span>") + escapeHtml(webControllerDescription(configurationService_.getLocale(), selected->implementation))
            + "</span></div></section>";
    }
    c += "<script>const thresholdMeasurementCatalog=" + measurementCatalog + ";function thresholdMeasurements(rebuild){const s=document.getElementById('thresholdSourceSensor'),m=document.getElementById('thresholdMeasurement'),previous=m.value;if(rebuild){m.replaceChildren(new Option((globalThis.EnvNodeUi.SelectAMeasurement),'unknown'));for(const x of thresholdMeasurementCatalog[s.value]||[]){const o=new Option(x.label,x.value);o.dataset.unit=x.unit;m.add(o)}if(Array.from(m.options).some(o=>o.value===previous))m.value=previous;else m.value='unknown'}const o=m.options[m.selectedIndex];document.getElementById('thresholdUnit').textContent=o?o.dataset.unit||'':''}function controllerFields(){const s=document.getElementById('controllerImplementation'),k=s.options[s.selectedIndex].dataset.kind;document.getElementById('blinkConfiguration').style.display=k==='blink'?'block':'none';document.getElementById('thresholdConfiguration').style.display=k==='threshold'?'block':'none';document.getElementById('selectorConfiguration').style.display=k==='selector'?'block':'none';document.getElementById('thresholdTargetField').style.display=document.getElementById('thresholdDecisionOnly').checked?'none':'block';thresholdMeasurements(false)}document.getElementById('controllerImplementation').addEventListener('change',controllerFields);document.getElementById('thresholdSourceSensor').addEventListener('change',()=>thresholdMeasurements(true));document.getElementById('thresholdMeasurement').addEventListener('change',()=>thresholdMeasurements(false));document.getElementById('thresholdDecisionOnly').addEventListener('change',controllerFields);controllerFields();</script>";
    sendPage(uiText(TextId::ConfigureControllerSlot), "/controllers", c);
}

void WebService::handleDiagnostics() {
    renderDiagnostics();
}

void WebService::handleI2CScan() {
    logger_.info("I2C scan started");
    i2cBusManager_.scanAll();
    renderDiagnostics();
    logger_.info("I2C scan complete");
}

void WebService::renderDiagnostics() {
    String c;
    const BoardProfile& board = currentBoardProfile();
    c.reserve(1800 + sensorManager_.sensorCount() * 450);
    c = (String("<div class='grid'><section class='card'><h2>") + uiText(TextId::System) + "</h2><div class='kv'><span>" + uiText(TextId::Uptime) + "</span><span>")+localeFormatter_.formatNumber(millis()/1000UL,0)+(String(" s</span><span>") + uiText(TextId::FreeHeap) + "</span><span>")+localeFormatter_.formatNumber(ESP.getFreeHeap(),0)+(((String("") + String(" ") + uiText(TextId::Bytes) + "</span></div></section><section class='card'><h2>")) + uiText(TextId::Hardware) + "</h2><div class='kv'><span>" + uiText(TextId::Board) + "</span><span>") + escapeHtml(board.displayName) + (String("</span><span>") + uiText(TextId::BoardRevision) + "</span><span>") + String(board.revision.major) + "." + String(board.revision.minor) + "</span><span>MCU</span><span>" + escapeHtml(ESP.getChipModel()) + (String("</span><span>") + uiText(TextId::CPUFrequency) + "</span><span>") + localeFormatter_.formatNumber(ESP.getCpuFreqMHz(), 0) + (String(" MHz</span><span>") + uiText(TextId::FlashSize) + "</span><span>")+localeFormatter_.formatNumber(ESP.getFlashChipSize(),0)+(((String("") + String(" ") + uiText(TextId::Bytes) + "</span></div></section><section class='card'><h2>")) + uiText(TextId::Services) + "</h2><div class='kv'><span>WiFi</span><span>")+(wifiService_.connected()?uiText(TextId::Connected):uiText(TextId::Disconnected))+(String("</span><span>") + uiText(TextId::MQTT) + "</span><span>")+(mqttService_.connected()?uiText(TextId::Connected):uiText(TextId::Disconnected))+(String("</span><span>") + uiText(TextId::Time) + "</span><span>")+(timeService_.synchronized()?uiText(TextId::Synchronized):uiText(TextId::Pending))+"</span></div></section></div>";
    c += (String("<section class='card'><h2>") + uiText(TextId::ICDiagnostics) + "</h2>");
    if (board.i2cBusCount == 0) {
        c += (String("<p>") + uiText(TextId::NoICBusesAvailableOnThisBoard) + "</p>");
    } else {
        c += (String("<p class='help'>") + uiText(TextId::ShowingTheLatestScanPerformedAutomaticallyAtBoot) + "</p>");
        for (size_t index = 0; index < board.i2cBusCount; ++index) {
            const BoardI2CBusCapability& bus = board.i2cBuses[index];
            c += "<h3>" + String(i2cBusName(bus.bus)) + "</h3><p class='secondary'>SDA GPIO"
                + String(bus.sda.number) + " · SCL GPIO" + String(bus.scl.number) + "</p>";
            const I2CScanResult* cachedScan = i2cBusManager_.lastScan(bus.bus);
            if (cachedScan == nullptr) {
                c += (String("<p>") + uiText(TextId::NotScanned) + "</p>");
                continue;
            }
            const I2CScanResult& result = *cachedScan;
            if (result.status == I2CScanStatus::BusUnavailable) {
                c += (String("<div class='notice error'>") + uiText(TextId::BusIsNotInitialized) + "</div>");
                continue;
            }
            if (result.addressCount == 0) {
                c += (String("<p>") + uiText(TextId::NoDevicesDetected) + "</p>");
            } else {
                c += (String("<div class='table-scroll'><table><thead><tr><th>") + uiText(TextId::Address) + "</th><th>" + uiText(TextId::DeviceModule) + "</th></tr></thead><tbody>");
                for (size_t addressIndex = 0; addressIndex < result.addressCount; ++addressIndex) {
                    char addressText[5];
                    snprintf(addressText, sizeof(addressText), "0x%02X", result.addresses[addressIndex]);
                    c += "<tr><td><code>" + String(addressText) + "</code></td><td>";
                    const ModuleDiscoveryResult* module = nullptr;
                    // IdentityEeprom24LC32 uses the board's identity bus, I2C0.
                    if (bus.bus == I2CBus::I2C0) {
                        for (size_t slotIndex = 0; slotIndex < ModuleDiscoveryService::SlotCount; ++slotIndex) {
                            const auto* candidate = moduleDiscoveryService_.result(static_cast<ModuleSlot>(slotIndex));
                            if (candidate != nullptr && candidate->eepromAddress == result.addresses[addressIndex]) {
                                module = candidate;
                                break;
                            }
                        }
                    }
                    if (bus.bus == I2CBus::I2C0 && result.addresses[addressIndex] == 0x50) {
                        c += uiText(TextId::MainboardEEPROM);
                        if (boardIdentityResolution_.source == BoardIdentitySource::EEPROM) {
                            const BoardIdentity& identity = boardIdentityResolution_.identity;
                            c += "<br>" + escapeHtml(board.displayName)
                                + (String(" · ") + uiText(TextId::Revision) + " ") + String(identity.revision.major) + "."
                                + String(identity.revision.minor);
                        }
                        c += (String("<span class='secondary'>") + uiText(TextId::BoardIdentity2) + " " + "")
                            + String(boardIdentityStatusName(boardIdentityResolution_.recordStatus))
                            + (String(" · ") + uiText(TextId::Source) + ": ") + boardIdentitySourceName(boardIdentityResolution_.source)
                            + "</span>";
                    } else if (module != nullptr) {
                        c += (String(uiText(TextId::ModuleEEPROMSlot)) + " ") + escapeHtml(moduleSlotName(module->slot));
                        if (module->identified()) {
                            c += "<br>" + escapeHtml(descriptorText(module->descriptorName.empty()
                                ? module->descriptorTypeId : module->descriptorName))
                                + (String(" · ") + uiText(TextId::Revision) + " ") + String(module->descriptorRevision.major) + "."
                                + String(module->descriptorRevision.minor);
                            c += (String("<span class='secondary'>") + uiText(TextId::Descriptor) + " " + "")
                                + String(hardwareDescriptorDecodeStatusName(module->descriptorStatus))
                                + " · " + hardwareDescriptorCompatibilityStatusName(module->descriptorCompatibility)
                                + "</span>";
                        } else {
                            c += (String("<span class='secondary'>") + uiText(TextId::Descriptor) + " " + "")
                                + String(module->descriptorStoreStatus == HardwareDescriptorStoreStatus::Valid
                                    ? hardwareDescriptorDecodeStatusName(module->descriptorStatus)
                                    : hardwareDescriptorStoreStatusName(module->descriptorStoreStatus)) + "</span>";
                        }
                    } else {
                        c += uiText(TextId::UnidentifiedICDevice);
                    }
                    c += "</td></tr>";
                }
                c += "</tbody></table></div>";
            }
            if (result.status == I2CScanStatus::CompleteWithProbeErrors) {
                c += (String("<div class='notice error'>") + uiText(TextId::TheScanEncountered) + " " + "")
                    + localeFormatter_.formatNumber(result.probeErrorCount, 0)
                    + (String("") + String(" ") + uiText(TextId::BusProbeErrorsSeeLogs) + "</div>");
            }
        }
        c += (String("<form method='post' action='/diagnostics/i2c/scan'><button type='submit'>") + uiText(TextId::ScanICBuses) + "</button></form>");
    }
    c += "</section>";
    for(size_t index=0;index<sensorManager_.sensorCount();++index){SensorRuntimeInfo i; if(!sensorManager_.runtimeInfo(index,i))continue;SensorRuntimeStatus s;sensorManager_.runtimeStatus(i.id,s);c+=(String("<section class='card'><h2>") + uiText(TextId::Sensor) + " " + "")+localeFormatter_.formatNumber(i.id,0)+" · "+escapeHtml(i.name)+(String("</h2><div class='kv'><span>") + uiText(TextId::Type) + "</span><span>")+escapeHtml(i.type)+(String("</span><span>") + uiText(TextId::State) + "</span><span>")+sensorStateName(i.state, configurationService_.getLocale())+(String("</span><span>") + uiText(TextId::Accepted) + "</span><span>")+localeFormatter_.formatNumber(s.acceptedMeasurementCount,0)+(String("</span><span>") + uiText(TextId::Rejected) + "</span><span>")+localeFormatter_.formatNumber(s.rejectedMeasurementCount,0)+(String("</span><span>") + uiText(TextId::PreSyncDiscarded) + "</span><span>")+localeFormatter_.formatNumber(s.preSyncDiscardCount,0)+(String("</span><span>") + uiText(TextId::LastSampleEmissions) + "</span><span>")+localeFormatter_.formatNumber(s.lastSampleEmissionCount,0)+"</span></div></section>";}
    sendPage(uiText(TextId::Diagnostics), "/diagnostics", c);
}


void WebService::handleMqttSave() {
    const String portText=server_.arg("port"); const long port=portText.toInt();
    bool ok=port>=1&&port<=65535&&configurationService_.setMqttServer(server_.arg("server"))&&configurationService_.setMqttPort(static_cast<uint16_t>(port))&&configurationService_.setMqttUsername(server_.arg("username"));
    if(ok&&server_.arg("changePassword")=="1")ok=configurationService_.setMqttPassword(server_.arg("password"));
    sendConfigurationResult(configurationSaveResult(ok,ConfigurationArea::Mqtt),uiText(TextId::MQTTSettingsSaved),uiText(TextId::MQTTSaveFailed),"/mqtt",uiText(TextId::InvalidMQTTConfiguration));
}

void WebService::handleDiscoveryRepublish() {
    switch (discoveryPublisher_.republish()) {
        case DiscoveryRepublishResult::Published:
            sendResult(uiText(TextId::HomeAssistantDiscoveryPublished), "/mqtt",
                uiText(TextId::HomeAssistantDiscoveryPublished2), true);
            return;
        case DiscoveryRepublishResult::MqttUnavailable:
            sendResult(uiText(TextId::HomeAssistantDiscoveryUnavailable), "/mqtt",
                uiText(TextId::MQTTIsUnavailableDiscoveryWasNotPublishedRetry), false);
            return;
        case DiscoveryRepublishResult::PublishFailed:
        default:
            sendResult(uiText(TextId::HomeAssistantDiscoveryFailed), "/mqtt",
                uiText(TextId::MQTTPublicationFailedDiscoveryWasNotUpdated), false);
            return;
    }
}


void WebService::handleUnitsSave() {
    PresentationUnit t,p,s,r; bool ok=parseUnit(server_.arg("temperature"),MeasurementType::Temperature,t)&&parseUnit(server_.arg("pressure"),MeasurementType::AtmosphericPressure,p)&&parseUnit(server_.arg("solarTemperature"),MeasurementType::SolarCellTemperature,s)&&parseUnit(server_.arg("rainLevel"),MeasurementType::RainDetectorLevel,r);
    if(ok)ok=configurationService_.setPresentationUnit(MeasurementType::Temperature,t)&&configurationService_.setPresentationUnit(MeasurementType::AtmosphericPressure,p)&&configurationService_.setPresentationUnit(MeasurementType::SolarCellTemperature,s)&&configurationService_.setPresentationUnit(MeasurementType::RainDetectorLevel,r);
    sendConfigurationResult(configurationSaveResult(ok,ConfigurationArea::PresentationUnits),uiText(TextId::PresentationUnitsSaved),uiText(TextId::UnitSaveFailed),"/units",uiText(TextId::InvalidOrUnsupportedPresentationUnit));
}

void WebService::handlePressureReductionSave() {
    PressureReductionConfiguration candidate;
    const bool ok = server_.hasArg("temperatureSource") && server_.hasArg("temperatureMaxAge")
        && decodePressureReduction(server_.arg("temperatureSource") + ";60000", candidate)
        && parseTemperatureAgeInput(server_.arg("temperatureMaxAge"), configurationService_.getLocale(), candidate.maxTemperatureAgeMs)
        && configurationService_.setPressureReduction(candidate);
    sendConfigurationResult(configurationSaveResult(ok, ConfigurationArea::GeoPosition),
        uiText(TextId::DeviceSettingsSaved), uiText(TextId::DeviceSaveFailed),
        "/device", uiText(TextId::PressureSettingsInvalid));
}

void WebService::handleGeoPositionSave() {
    GeoPosition position;
    const bool ok = server_.hasArg("latitude") && server_.hasArg("longitude") && server_.hasArg("altitude")
        && parseGeoPositionInput(server_.arg("latitude"), server_.arg("longitude"), server_.arg("altitude"), configurationService_.getLocale(), position)
        && configurationService_.setGeoPosition(position);
    sendConfigurationResult(configurationSaveResult(ok, ConfigurationArea::GeoPosition),
        uiText(TextId::DeviceSettingsSaved), uiText(TextId::DeviceSaveFailed),
        "/device", uiText(TextId::InvalidGeoPosition));
}


void WebService::handleSensorSave() {
    const long requestedSlot = server_.arg("slot").toInt();
    const SensorImplementationMetadata* metadata =
        SensorImplementationRegistry::findByStableId(server_.arg("implementation").c_str());
    bool ok = requestedSlot >= 1
        && requestedSlot <= static_cast<long>(MaxSensorSlotCount)
        && metadata != nullptr;
    SensorSlotConfiguration slot;
    if (ok) {
        slot = configurationService_.getConfiguration().sensorSlots[requestedSlot - 1];
        slot.enabled = server_.hasArg("enabled") && server_.arg("enabled") == "1";
        slot.name = server_.arg("name");
        slot.implementation = metadata->implementation;
        slot.schedule = metadata->defaultSchedule;
        slot.schedule.enabled = slot.enabled;
        slot.hardware = HardwareResourceAssignment::none();
        if (metadata->defaultSchedule.acquisitionMode == AcquisitionMode::Periodic) {
            const long long interval = strtoll(server_.arg("interval").c_str(), nullptr, 10);
            if (interval <= 0 || interval > 0x7FFFFFFFLL) ok = false;
            else slot.schedule.sampleIntervalMs = static_cast<uint32_t>(interval);
        } else {
            slot.schedule.sampleIntervalMs = 0;
        }
        if (slot.implementation == SensorImplementation::AM2302
            || slot.implementation == SensorImplementation::RainGauge) {
            const long gpio = server_.arg("gpio").toInt();
            if (gpio < 0 || gpio > 255) ok = false;
            else {
                const GpioResource resource(static_cast<uint8_t>(gpio));
                slot.hardware = HardwareResourceAssignment::gpioResource(resource);
                if (slot.implementation == SensorImplementation::AM2302) {
                    slot.implementationConfiguration.am2302 = AM2302Configuration(resource);
                } else {
                    char* millimetersEnd = nullptr;
                    const String millimetersText = server_.arg("millimetersPerTip");
                    const float millimetersPerTip = strtof(
                        millimetersText.c_str(), &millimetersEnd);
                    const long debounceMs = server_.arg("debounceMs").toInt();
                    if (millimetersEnd == nullptr || *millimetersEnd != '\0'
                        || !std::isfinite(millimetersPerTip)
                        || debounceMs < 1 || debounceMs > 5000) {
                        ok = false;
                    } else {
                        slot.implementationConfiguration.rainGauge = RainGaugeConfiguration(
                            resource, millimetersPerTip, static_cast<uint32_t>(debounceMs));
                    }
                }
            }
        }
        if (slot.implementation == SensorImplementation::BME280
            || slot.implementation == SensorImplementation::SHT4x
            || slot.implementation == SensorImplementation::SHTC3) {
            const long address = server_.arg("i2cAddress").toInt();
            const long busValue = server_.arg("i2cBus").toInt();
            const I2CBus bus = static_cast<I2CBus>(busValue);
            const bool validAddress = slot.implementation == SensorImplementation::SHT4x
                ? address == 0x44
                : slot.implementation == SensorImplementation::SHTC3
                    ? address == 0x70
                    : (address == 0x76 || address == 0x77);
            if (!validAddress
                || busValue < 0 || busValue > 255
                || BoardCapabilities::current().i2cBus(bus) == nullptr) {
                ok = false;
            } else {
                const I2CResource resource(bus, static_cast<uint8_t>(address));
                slot.hardware = HardwareResourceAssignment::i2cResource(resource);
                slot.implementationConfiguration.bme280 = BME280Configuration(resource);
                slot.implementationConfiguration.sht4x = SHT4xConfiguration(resource);
                slot.implementationConfiguration.shtc3 = SHTC3Configuration(resource);
            }
        }
    }
    if (ok) ok = configurationService_.setSensorSlotConfiguration(slot);
    sendConfigurationResult(
        configurationSaveResult(ok, ConfigurationArea::Sensors),
        uiText(TextId::SensorSlotSaved),
        uiText(TextId::SensorSlotSaveFailed),
        "/sensors",
        uiText(TextId::InvalidSlotConfigurationScheduleHardwareResourceOrResource));
}
void WebService::handleSensorApply() {
    if (runtimeManager_.pendingAction() != RuntimeAction::RestartSensorManager) {
        sendResult(
            uiText(TextId::SensorChangesNotApplied),
            "/sensors",
            uiText(TextId::NoSensorManagerRestartIsPendingOrA),
            false);
        return;
    }
    if (!runtimeManager_.applyPendingSensorChanges()) {
        sendResult(
            uiText(TextId::SensorChangesNotApplied),
            "/sensors",
            uiText(TextId::TheSensorRuntimeRebuildFailedThePendingAction),
            false);
        return;
    }
    sendResult(
        uiText(TextId::SensorChangesApplied),
        "/sensors",
        uiText(TextId::TheCompleteSensorRuntimeCompositionWasRebuiltSuccessfully),
        true);
}
void WebService::handleActuatorSave() {
    const long requestedSlot = server_.arg("slot").toInt();
    if (requestedSlot >= 1 && requestedSlot <= static_cast<long>(MaxActuatorSlotCount)) {
        ActuatorSlotConfiguration slot = configurationService_.getConfiguration().actuatorSlots[requestedSlot - 1];
        ModuleActuatorReference reference = slot.moduleTarget;
        ActuatorRuntimeInfo runtime;
        const bool descriptorRuntime = actuatorInfoById(actuatorRuntime_, slot.slotId, runtime)
            && runtime.origin == ActuatorRuntimeOrigin::ModuleDescriptor;
        const bool module = validModuleActuatorReference(reference)
            || actuatorRuntime_.moduleReference(slot.slotId, reference);
        if (module || descriptorRuntime || server_.hasArg("module")) {
            bool ok = module && server_.arg("module") == moduleActuatorToken(reference);
            slot.moduleTarget = reference;
            slot.name = server_.arg("name");
            slot.enabled = server_.arg("enabled") == "1";
            slot.implementation = ActuatorImplementation::GpioOnOff;
            slot.hardware = HardwareResourceAssignment::none();
            if (ok) ok = configurationService_.setActuatorSlotConfiguration(slot);
            sendConfigurationResult(configurationSaveResult(ok, ConfigurationArea::Actuators),
                uiText(TextId::ModuleActuatorSaved), uiText(TextId::ModuleActuatorSaveFailed), "/actuators",
                uiText(TextId::InvalidConfigurationOrModuleIdentityChangedReloadThe));
            return;
        }
    }
    const ActuatorImplementationMetadata* metadata =
        ActuatorImplementationRegistry::findByStableId(
            server_.arg("implementation").c_str());
    bool ok = requestedSlot >= 1
        && requestedSlot <= static_cast<long>(MaxActuatorSlotCount)
        && metadata != nullptr;
    ActuatorSlotConfiguration slot;
    if (ok) {
        slot = configurationService_.getConfiguration().actuatorSlots[requestedSlot - 1];
        slot.enabled = server_.hasArg("enabled") && server_.arg("enabled") == "1";
        slot.name = server_.arg("name");
        slot.implementation = metadata->implementation;
        slot.hardware = HardwareResourceAssignment::none();
        if (metadata->interfaceKind == HardwareInterfaceKind::GPIO) {
            const long gpio = server_.arg("gpio").toInt();
            if (gpio < 0 || gpio > 255) ok = false;
            else slot.hardware = HardwareResourceAssignment::gpioResource(
                GpioResource(static_cast<uint8_t>(gpio)));
        }
    }
    if (ok && actuatorRuntime_.moduleOwnsHardware(slot.hardware)) ok = false;
    if (ok) ok = configurationService_.setActuatorSlotConfiguration(slot);
    sendConfigurationResult(
        configurationSaveResult(ok, ConfigurationArea::Actuators),
        uiText(TextId::ActuatorSlotSaved),
        uiText(TextId::ActuatorSlotSaveFailed),
        "/actuators",
        uiText(TextId::InvalidSlotConfigurationHardwareResourceOrResourceConflict));
}
void WebService::handleActuatorOn() { handleActuatorState(OnOffState::On); }
void WebService::handleActuatorOff() { handleActuatorState(OnOffState::Off); }
void WebService::handleActuatorState(OnOffState state) {
    const long requestedSlot = server_.arg("slot").toInt();
    if (requestedSlot < 1 || requestedSlot > static_cast<long>(MaxActuatorSlotCount)) {
        sendResult(uiText(TextId::ActuatorCommandFailed), "/actuators",
            uiText(TextId::TheRequestedActuatorSlotDoesNotExist), false);
        return;
    }
    IOnOffActuator* actuator = actuatorRuntime_.onOffActuator(
        static_cast<ActuatorId>(requestedSlot));
    if (actuator == nullptr) {
        sendResult(uiText(TextId::ActuatorCommandFailed), "/actuators",
            uiText(TextId::NoInitializedOnOffActuatorIsActiveFor), false);
        return;
    }
    const ActuatorOperationResult result = actuator->setState(state);
    if (result != ActuatorOperationResult::Completed) {
        sendResult(uiText(TextId::ActuatorCommandFailed), "/actuators",
            actuatorOperationFailure(result, configurationService_.getLocale()), false);
        return;
    }
    sendSeeOther(server_, "/actuators");
}
void WebService::handleActuatorLevel() {
    const long requestedSlot = server_.arg("slot").toInt();
    const String levelText = server_.arg("level");
    char* levelEnd = nullptr;
    const long requestedLevel = strtol(levelText.c_str(), &levelEnd, 10);
    if (requestedSlot < 1
        || requestedSlot > static_cast<long>(MaxActuatorSlotCount)
        || levelText.isEmpty()
        || levelEnd == levelText.c_str()
        || *levelEnd != '\0'
        || requestedLevel < ActuatorLevel::Minimum
        || requestedLevel > ActuatorLevel::Maximum) {
        sendResult(uiText(TextId::ActuatorLevelCommandFailed), "/actuators",
            uiText(TextId::TheRequestedActuatorSlotOrLevelIsInvalid), false);
        return;
    }
    ILevelActuator* actuator = actuatorRuntime_.levelActuator(
        static_cast<ActuatorId>(requestedSlot));
    if (actuator == nullptr) {
        sendResult(uiText(TextId::ActuatorLevelCommandFailed), "/actuators",
            uiText(TextId::NoInitializedLevelCapableActuatorIsActiveFor), false);
        return;
    }
    ActuatorLevel level = ActuatorLevel::off();
    if (!ActuatorLevel::tryCreate(static_cast<uint8_t>(requestedLevel), level)
        || actuator->setLevel(level) != ActuatorOperationResult::Completed) {
        sendResult(uiText(TextId::ActuatorLevelCommandFailed), "/actuators",
            uiText(TextId::TheLevelCapableActuatorRejectedTheOperation), false);
        return;
    }
    sendSeeOther(server_, "/actuators");
}
void WebService::handleActuatorApply() {
    if (runtimeManager_.pendingAction() != RuntimeAction::RestartActuatorRuntime) {
        sendResult(uiText(TextId::ActuatorChangesNotApplied), "/actuators",
            uiText(TextId::NoActuatorRuntimeApplyIsPendingOrA),
            false);
        return;
    }
    if (!runtimeManager_.applyPendingActuatorChanges(
            configurationService_.getConfiguration().actuatorSlots)) {
        sendResult(uiText(TextId::ActuatorChangesNotApplied), "/actuators",
            uiText(TextId::TheActuatorRuntimeRebuildFailedThePreviousComposition),
            false);
        return;
    }
    sendSeeOther(server_, "/actuators");
}
void WebService::handleControllerSave() {
    const long requestedSlot = server_.arg("slot").toInt();
    const ControllerImplementationMetadata* metadata =
        ControllerImplementationRegistry::findByStableId(
            server_.arg("implementation").c_str());
    bool ok = requestedSlot >= 1
        && requestedSlot <= static_cast<long>(MaxControllerSlotCount)
        && metadata != nullptr;
    ControllerSlotConfiguration slot;
    if (ok) {
        slot = configurationService_.getConfiguration().controllerSlots[requestedSlot - 1];
        slot.enabled = server_.hasArg("enabled") && server_.arg("enabled") == "1";
        slot.name = server_.arg("name");
        slot.implementation = metadata->implementation;
        if (slot.implementation == ControllerImplementation::Blink) {
            const String targetText = server_.arg("targetActuator");
            const bool moduleTarget = targetText.startsWith("m:");
            const bool currentModuleTarget = targetText == "m:current"
                && validModuleActuatorReference(slot.moduleTarget);
            const long target = currentModuleTarget
                ? slot.implementationConfiguration.blink.targetActuatorId : moduleTarget
                ? targetText.substring(2).toInt() : targetText.toInt();
            uint32_t onDuration = 0;
            uint32_t offDuration = 0;
            if (target < 1 || target > static_cast<long>(MaxActuatorSlotCount)
                || !parseControllerDuration(server_.arg("onDuration"), onDuration)
                || !parseControllerDuration(server_.arg("offDuration"), offDuration)) {
                ok = false;
            } else {
                slot.implementationConfiguration.blink.targetActuatorId =
                    static_cast<ActuatorId>(target);
                if (!currentModuleTarget) slot.moduleTarget = ModuleActuatorReference{};
                if (moduleTarget && !currentModuleTarget) {
                    ActuatorRuntimeInfo runtime;
                    bool found = false;
                    for (size_t index = 0; index < actuatorRuntime_.runtimeCount(); ++index) {
                        if (actuatorRuntime_.runtimeInfo(index, runtime)
                            && runtime.id == target
                            && runtime.origin == ActuatorRuntimeOrigin::ModuleDescriptor
                            && actuatorRuntime_.moduleReference(
                                runtime.id, slot.moduleTarget)) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) ok = false;
                }
                slot.implementationConfiguration.blink.onDurationMs = onDuration;
                slot.implementationConfiguration.blink.offDurationMs = offDuration;
            }
        } else if (slot.implementation == ControllerImplementation::Threshold) {
            const bool decisionOnly = server_.arg("decisionOnly") == "1";
            const String targetText = decisionOnly ? String("1") : server_.arg("thresholdTargetActuator");
            const bool moduleTarget = targetText.startsWith("m:");
            const bool currentModuleTarget = targetText == "m:current"
                && validModuleActuatorReference(slot.moduleTarget);
            const String parsedTarget = currentModuleTarget
                ? String(slot.implementationConfiguration.threshold.targetActuatorId)
                : moduleTarget
                ? targetText.substring(2) : targetText;
            ok = applyThresholdControllerWebFields(
                server_.arg("thresholdSourceSensor"),
                server_.arg("thresholdMeasurement"),
                parsedTarget,
                server_.arg("onThreshold"),
                server_.arg("offThreshold"),
                server_.arg("thresholdDirection"),
                server_.arg("maxMeasurementAge"),
                slot);
            if (ok) {
                ThresholdControllerConfiguration& threshold =
                    slot.implementationConfiguration.threshold;
                threshold.decisionOnly = decisionOnly;
                if (decisionOnly) threshold.targetActuatorId = InvalidActuatorId;
                if (!currentModuleTarget) slot.moduleTarget = ModuleActuatorReference{};
                if (moduleTarget && !currentModuleTarget) {
                    ActuatorRuntimeInfo runtime;
                    bool found = false;
                    for (size_t index = 0; index < actuatorRuntime_.runtimeCount(); ++index) {
                        if (actuatorRuntime_.runtimeInfo(index, runtime)
                            && runtime.id == threshold.targetActuatorId
                            && runtime.origin == ActuatorRuntimeOrigin::ModuleDescriptor
                            && actuatorRuntime_.moduleReference(
                                runtime.id, slot.moduleTarget)) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) ok = false;
                }
            }
        }
    }
    if (ok && slot.implementation == ControllerImplementation::Selector) ok = readSelectorConfiguration(slot);
    if (ok) ok = configurationService_.setControllerSlotConfiguration(slot);
    sendConfigurationResult(
        configurationSaveResult(ok, ConfigurationArea::Controllers),
        uiText(TextId::ControllerSlotSaved),
        uiText(TextId::ControllerSlotSaveFailed),
        "/controllers",
        uiText(TextId::InvalidControllerConfigurationSelectAnUnclaimedTargetSelectors));
}
void WebService::handleControllerApply() {
    if (runtimeManager_.pendingAction() != RuntimeAction::RestartControllerRuntime) {
        sendResult(uiText(TextId::ControllerChangesNotApplied), "/controllers",
            uiText(TextId::NoControllerRuntimeApplyIsPendingOrA),
            false);
        return;
    }
    if (!runtimeManager_.applyPendingControllerChanges(
            configurationService_.getConfiguration().controllerSlots)) {
        sendResult(uiText(TextId::ControllerChangesNotApplied), "/controllers",
            uiText(TextId::TheControllerRuntimeRebuildFailedThePreviousComposition),
            false);
        return;
    }
    sendSeeOther(server_, "/controllers");
}
void WebService::handleControllerStart() {
    handleControllerRuntimeOperation(true);
}
void WebService::handleControllerStop() {
    handleControllerRuntimeOperation(false);
}
void WebService::handleControllerRuntimeOperation(bool start) {
    const long requestedSlot = server_.arg("slot").toInt();
    if (requestedSlot < 1 || requestedSlot > static_cast<long>(MaxControllerSlotCount)) {
        sendResult(uiText(TextId::ControllerCommandFailed), "/controllers",
            uiText(TextId::TheRequestedControllerSlotDoesNotExist), false);
        return;
    }
    const ControllerId id = static_cast<ControllerId>(requestedSlot);
    const ControllerOperationResult result = start
        ? controllerRuntime_.startController(id)
        : controllerRuntime_.stopController(id);
    if (result != ControllerOperationResult::Completed
        && result != ControllerOperationResult::NoAction) {
        String message = start ? (String(uiText(TextId::TheControllerCouldNotBeStarted)) + " ")
            : (String(uiText(TextId::TheControllerCouldNotBeStoppedCleanly)) + " ");
        message += controllerOperationName(result, configurationService_.getLocale());
        message += ".";
        sendResult(uiText(TextId::ControllerCommandFailed), "/controllers", message.c_str(), false);
        return;
    }
    sendSeeOther(server_, "/controllers");
}


void WebService::handleNotFound() { server_.send(404,"text/plain",uiText(TextId::NotFound)); }


Locale WebService::webLocale() const { return configurationService_.getLocale(); }
void WebService::networkSettingsSaved(bool success) {
    const ConfigurationSaveResult result = configurationSaveResult(success, ConfigurationArea::Network);
    sendConfigurationResult(result, uiText(TextId::NetworkSettingsSaved), uiText(TextId::NetworkSaveFailed),
        "/network", uiText(TextId::InvalidNetworkSettingsStaticModeRequiresAValid));
}

void WebService::sendResult(const char* title, const char* route, const char* message, bool success) { sendWebResult(*this, title, route, message, success); }

String WebService::renderHardwareOverview() {
    String c;
    c.reserve(1500);
    const BoardProfile& board = currentBoardProfile();
    const BoardIdentity& identity = boardIdentityResolution_.identity;
    const bool recordFormatKnown =
        boardIdentityResolution_.recordStatus == BoardIdentityStatus::Valid
        || boardIdentityResolution_.recordStatus == BoardIdentityStatus::UnassignedSerial;
    const String serialNumber = !isBoardSerialNumberAssigned(identity.serialNumber)
        ? String(uiText(TextId::Unassigned))
        : String(identity.serialNumber.value);
    c = String();
    c += (String("<section class='card'><h2>") + uiText(TextId::BoardIdentity) + "</h2><div class='kv'>");
    c += (String("<span>") + uiText(TextId::Source) + "</span><span>") + String(boardIdentitySourceName(boardIdentityResolution_.source)) + "</span>";
    c += (String("<span>") + uiText(TextId::RecordStatus) + "</span><span>") + String(boardIdentityStatusName(boardIdentityResolution_.recordStatus)) + "</span>";
    c += (String("<span>") + uiText(TextId::BoardProfile) + "</span><span>") + escapeHtml(board.displayName) + "</span>";
    c += (String("<span>") + uiText(TextId::BoardProfileId) + "</span><span>") + String(static_cast<unsigned int>(identity.profileId)) + "</span>";
    c += (String("<span>") + uiText(TextId::HardwareRevision) + "</span><span>") + String(identity.revision.major) + "." + String(identity.revision.minor) + "</span>";
    c += (String("<span>") + uiText(TextId::SerialNumber) + "</span><span>") + serialNumber + "</span>";
    c += (String("<span>") + uiText(TextId::EEPROMFormat) + "</span><span>") + String(recordFormatKnown ? "1" : "—") + "</span>";
    c += (String("<span>") + uiText(TextId::NormalRuntime) + "</span><span>") + String(boardIdentityResolution_.normalRuntimeAllowed ? uiText(TextId::Allowed) : uiText(TextId::Blocked)) + "</span>";
    c += "</div>";
    if (boardIdentityResolution_.source == BoardIdentitySource::BuildFallback) {
        c += (String("<div class='notice'>") + uiText(TextId::NoUsableEEPROMBoardIdentityWasSelectedThis) + "</div>");
    }
    if (boardIdentityResolution_.recordStatus == BoardIdentityStatus::UnassignedSerial) {
        c += (String("<div class='notice'>") + uiText(TextId::TheEEPROMBoardIdentityIsValidButIts) + "</div>");
    }
    c += "</section>";
    c += (((String("<section class='card'><h2>") + uiText(TextId::ModuleIdentity) + "</h2><div class='scroll'><table><thead><tr><th>" + uiText(TextId::Slot) + "</th><th>EEPROM</th><th>")) + uiText(TextId::Source) + (String("</th><th>") + uiText(TextId::Status) + "</th><th>" + uiText(TextId::Module) + "</th><th>" + uiText(TextId::Revision) + "</th><th>" + uiText(TextId::InstanceUUID) + "</th><th>" + uiText(TextId::SerialNumber) + "</th><th>" + uiText(TextId::ProductionBatch) + "</th><th>" + uiText(TextId::ProductionDate) + "</th></tr></thead><tbody>"));
    for (size_t index = 0; index < ModuleDiscoveryService::SlotCount; ++index) {
        const ModuleSlot slot = static_cast<ModuleSlot>(index);
        const ModuleDiscoveryResult* module = moduleDiscoveryService_.result(slot);
        if (module == nullptr) continue;
        const bool descriptor = module->source == ModuleDiscoverySource::Descriptor;
        const String moduleName = descriptor
            ? escapeHtml(descriptorText(module->descriptorName.empty()
                ? module->descriptorTypeId : module->descriptorName))
            : String("—");
        const String revision = descriptor && module->identified()
            ? String(module->descriptorRevision.major) + "." + String(module->descriptorRevision.minor)
            : String("—");
        const String moduleSerial = descriptor
            ? (module->descriptorSerialNumber.empty()
                ? String(uiText(TextId::Unassigned)) : escapeHtml(descriptorText(module->descriptorSerialNumber)))
            : String("—");
        const String instanceId = descriptor && module->descriptorHasInstanceId
            ? descriptorUuid(module->descriptorInstanceId) : String("—");
        const String productionBatch = descriptor
            && !module->descriptorProductionBatch.empty()
            ? escapeHtml(descriptorText(module->descriptorProductionBatch)) : String("—");
        const String productionDate = descriptor
            && !module->descriptorProductionDate.empty()
            ? escapeHtml(descriptorText(module->descriptorProductionDate)) : String("—");
        const char* status = descriptor
            ? (module->descriptorStoreStatus == HardwareDescriptorStoreStatus::Valid
                ? hardwareDescriptorDecodeStatusName(module->descriptorStatus)
                : hardwareDescriptorStoreStatusName(module->descriptorStoreStatus))
            : hardwareDescriptorStoreStatusName(module->descriptorStoreStatus);
        c += "<tr><td>" + String(moduleSlotName(slot)) + "</td><td>0x";
        if (module->eepromAddress < 0x10) c += "0";
        c += String(module->eepromAddress, HEX) + "</td><td>";
        c += moduleDiscoverySourceName(module->source);
        c += "</td><td>";
        c += status;
        c += "</td><td>" + moduleName + "</td><td>" + revision;
        c += "</td><td class='nowrap'>" + instanceId + "</td><td>" + moduleSerial;
        c += "</td><td>" + productionBatch + "</td><td>" + productionDate + "</td></tr>";
    }
    c += "</tbody></table></div></section>";
    c += (((String("<section class='card'><h2>") + uiText(TextId::DiscoveredModuleDevices) + "</h2><div class='scroll'><table><thead><tr><th>" + uiText(TextId::Slot) + "</th><th>")) + uiText(TextId::Device) + (String("</th><th>") + uiText(TextId::Kind) + "</th><th>" + uiText(TextId::Driver) + "</th><th>" + uiText(TextId::Capability) + "</th><th>" + uiText(TextId::Binding) + "</th><th>" + uiText(TextId::BoardResource) + "</th><th>" + uiText(TextId::Parameters) + "</th><th>" + uiText(TextId::Status) + "</th></tr></thead><tbody>"));
    // Keep the large inventory off both static DRAM and the HTTP task stack.
    std::unique_ptr<ModuleDeviceInventoryEntry[]> moduleDeviceInventory(
        new (std::nothrow) ModuleDeviceInventoryEntry[MaximumDescriptorDevices]);
    bool hasModuleDevices = false;
    for (size_t slotIndex = 0; slotIndex < ModuleDiscoveryService::SlotCount; ++slotIndex) {
        const ModuleSlot slot = static_cast<ModuleSlot>(slotIndex);
        const ModuleDiscoveryResult* module = moduleDiscoveryService_.result(slot);
        if (module == nullptr || !module->identified()
            || module->descriptorCompatibility != HardwareDescriptorCompatibilityStatus::Compatible) {
            continue;
        }
        size_t entryCount = 0;
        if (!moduleDeviceInventory || !deriveModuleDeviceInventory(
                module->descriptor, currentBoardProfile(), slot,
                moduleDeviceInventory.get(), MaximumDescriptorDevices, entryCount)) {
            continue;
        }
        for (size_t entryIndex = 0; entryIndex < entryCount; ++entryIndex) {
            hasModuleDevices = true;
            const ModuleDeviceInventoryEntry& entry = moduleDeviceInventory[entryIndex];
            String bindingText;
            String resourceText;
            for (size_t bindingIndex = 0; bindingIndex < entry.bindingCount; ++bindingIndex) {
                const ModuleDeviceBindingResolution& binding = entry.bindings[bindingIndex];
                if (bindingIndex > 0) { bindingText += "<br>"; resourceText += "<br>"; }
                bindingText += escapeHtml(descriptorText(binding.name));
                bindingText += " → ";
                bindingText += escapeHtml(descriptorText(binding.logicalResource.empty()
                    ? binding.target : binding.logicalResource));
                if (!binding.resolved) resourceText += "—";
                else if (binding.kind == HardwareDescriptorResourceKind::Gpio) {
                    resourceText += "GPIO";
                    resourceText += String(binding.gpio.number);
                } else if (binding.kind == HardwareDescriptorResourceKind::I2C) {
                    resourceText += i2cBusName(binding.i2cBus);
                } else if (binding.kind == HardwareDescriptorResourceKind::Power) {
                    resourceText += "+5V";
                } else resourceText += uiText(TextId::Resolved);
            }
            String parameters = "—";
            if (entry.hasActiveLevel || entry.hasSafeLevel) {
                parameters = "active=";
                parameters += entry.hasActiveLevel
                    ? (entry.activeLevelHigh ? "high" : "low") : "—";
                parameters += ", safe=";
                parameters += entry.hasSafeLevel
                    ? (entry.safeLevelHigh ? "high" : "low") : "—";
            }
            const String capability = entry.capabilities.contains(
                HardwareDescriptorCapabilityCode::ActuatorOnOff)
                ? String("actuator.on-off") : String("—");
            c += "<tr><td>" + String(moduleSlotName(slot)) + "</td><td>";
            c += escapeHtml(descriptorText(entry.id));
            c += "</td><td>" + String((entry.kind == HardwareDescriptorDeviceKind::Sensor ? uiText(TextId::Sensor) : entry.kind == HardwareDescriptorDeviceKind::Actuator ? uiText(TextId::Actuator) : uiText(TextId::Infrastructure)));
            c += "</td><td>" + String(moduleDeviceDriverName(entry.driver));
            c += " v" + String(entry.driver.apiVersion);
            c += "</td><td>" + capability + "</td><td>" + bindingText;
            c += "</td><td>" + resourceText + "</td><td>" + parameters;
            c += "</td><td>" + String(moduleDeviceInventoryStatusName(entry.status));
            c += "</td></tr>";
        }
    }
    if (!moduleDeviceInventory) {
        c += (String("<tr><td colspan='9'>") + uiText(TextId::InsufficientMemoryToRenderModuleDeviceInventory) + "</td></tr>");
    } else if (!hasModuleDevices) {
        c += (String("<tr><td colspan='9'>") + uiText(TextId::NoCompatibleDescriptorDefinedDevicesDiscovered) + "</td></tr>");
    }
    moduleDeviceInventory.reset();
    c += (String("</tbody></table></div><p class='help'>") + uiText(TextId::ThisIsAReadOnlyInventoryDerivedFrom) + "</p></section>");
    return c;
}
BoardIdentity WebService::provisioningBoardIdentity() const { return boardIdentityResolution_.identity; }
size_t WebService::provisioningProfileCount() const { return boardIdentityProfileCount(); }
bool WebService::provisioningProfileAt(size_t index, ProvisioningBoardProfile& result) const {
    const auto* p=boardIdentityProfileAt(index);
    if(!p)return false;
    result={p->id,p->displayName,p->revision};return true;
}
bool WebService::provisioningProfile(BoardProfileId id, ProvisioningBoardProfile& result) const {
    const auto* p=defaultBoardIdentityProfile(id);
    return p && provisioningProfile(id,p->revision,result);
}
bool WebService::provisioningProfile(BoardProfileId id, BoardRevision revision, ProvisioningBoardProfile& result) const {
    const auto* p=boardIdentityProfile(id,revision);
    if(!p || !boardProfile(id,revision))return false;
    result={p->id,p->displayName,p->revision};return true;
}
BoardProvisioningResult WebService::provisionBoardIdentity(const BoardIdentity& identity, bool confirmed) {
    return boardProvisioningService_.provision(identity, confirmed);
}
ModuleDescriptorProvisioningResult WebService::provisionModuleDescriptor(ModuleSlot slot,
    const uint8_t* payload, size_t size, bool confirmed) {
    return moduleDescriptorProvisioningService_.provision(slot, payload, size, confirmed);
}
String WebService::renderDeviceExtras() {
    String c;
    c += buildPressureReductionConfigurationHtml(configurationService_.getConfiguration(), configurationService_.getLocale());
    c += buildGeoPositionConfigurationHtml(configurationService_.getConfiguration().geoPosition,
        configurationService_.getLocale());
    c += "<section class='card'><h2>" + String(uiText(TextId::Hardware)) + "</h2><div class='kv'><span>"
        + uiText(TextId::BoardProfile) + "</span><span>" + escapeHtml(currentBoardProfile().displayName)
        + "</span><span>" + uiText(TextId::SerialNumber) + "</span><span>"
        + escapeHtml(boardIdentityResolution_.identity.serialNumber.value) + "</span></div><div class='actions'><a class='button' href='/hardware'>"
        + uiText(TextId::ViewHardware) + "</a></div></section>";
    return c;
}
} // namespace EnvNode
