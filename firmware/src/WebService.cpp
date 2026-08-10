#include "WebService.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_netif.h>
#include <cmath>
#include <cstdlib>
#include "FirmwareVersion.h"
#include "FirmwareBuildInfo.h"
#include "UnitConverter.h"
#include "SensorImplementationRegistry.h"
#include "SensorSlotConfiguration.h"

namespace WeatherStation {
namespace {

const char SharedStyle[] PROGMEM = R"CSS(
:root{--bg:#f3f6f8;--panel:#fff;--ink:#17212b;--muted:#637282;--line:#dbe3e8;--brand:#176b87;--brand2:#0f536a;--good:#177245;--warn:#a55b00;--bad:#a32828}html,body,*,*::before,*::after{box-sizing:border-box}html,body{max-width:100%}body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.45 system-ui,-apple-system,sans-serif;overflow-x:hidden}.shell{min-height:100vh;min-width:0;display:grid;grid-template-columns:220px minmax(0,1fr)}.side{background:#123644;color:#fff;padding:22px 16px;min-width:0}.brand{font-weight:750;font-size:19px;margin:0 8px 4px;overflow-wrap:anywhere}.version{color:#b8d0da;font-size:12px;margin:0 8px 20px}.nav a{display:block;color:#dbeaf0;text-decoration:none;padding:9px 11px;border-radius:7px;margin:2px 0}.nav a:hover,.nav a.active{background:#1d5367;color:#fff}.main{padding:28px;max-width:1100px;width:100%;min-width:0}.top{display:flex;justify-content:space-between;gap:16px;align-items:start;margin-bottom:22px;min-width:0}h1{font-size:25px;margin:0;overflow-wrap:anywhere}h2{font-size:17px;margin:0 0 14px}p{margin:8px 0;overflow-wrap:anywhere}.muted,.help{color:var(--muted)}.help{font-size:13px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(240px,100%),1fr));gap:16px;min-width:0;max-width:100%}.card{background:var(--panel);border:1px solid var(--line);border-radius:10px;padding:18px;margin-bottom:16px;box-shadow:0 1px 2px #1122;min-width:0;max-width:100%;overflow:hidden}.kv{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1.4fr);gap:8px 14px;min-width:0;max-width:100%}.kv>span{min-width:0;max-width:100%;overflow-wrap:anywhere}.kv span:nth-child(odd){color:var(--muted)}.badge{display:inline-block;border-radius:99px;padding:3px 9px;font-size:12px;font-weight:700;background:#e8edf0;max-width:100%;white-space:normal;overflow-wrap:anywhere}.badge.good{color:var(--good);background:#e2f4ea}.badge.warn{color:var(--warn);background:#fff0d7}.badge.bad{color:var(--bad);background:#fbe3e3}.notice{border-left:4px solid var(--warn);background:#fff8e9;padding:11px 13px;border-radius:5px;margin-bottom:16px;max-width:100%;overflow-wrap:anywhere}.success{border-left-color:var(--good);background:#eaf7ef}.error{border-left-color:var(--bad);background:#fdecec}label{display:block;font-weight:650;margin:0 0 14px;min-width:0;max-width:100%;overflow-wrap:anywhere}input,select{display:block;width:100%;max-width:520px;min-width:0;margin-top:5px;padding:9px 10px;border:1px solid #bfcbd2;border-radius:6px;background:#fff;color:var(--ink);font:inherit}input[type=checkbox],input[type=radio]{display:inline;width:auto;margin:0 7px 0 0}.choice{font-weight:500;margin:7px 0}.actions{display:flex;gap:10px;flex-wrap:wrap;margin-top:18px;min-width:0}button,.button{border:0;border-radius:6px;padding:9px 15px;background:var(--brand);color:#fff;text-decoration:none;font:600 14px inherit;cursor:pointer;max-width:100%;white-space:normal}button:hover,.button:hover{background:var(--brand2)}button.danger{background:var(--bad)}table{width:100%;border-collapse:collapse;font-size:14px}th,td{text-align:left;padding:9px;border-bottom:1px solid var(--line);vertical-align:top;overflow-wrap:anywhere}th{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.03em}.sensor-technical,.sensor-last-measurement,.sensor-actions,.sensor-actions .button{white-space:nowrap;overflow-wrap:normal}.sensor-technical,.sensor-last-measurement,.sensor-actions{width:1%}.scroll{overflow-x:auto;max-width:100%;min-width:0}details{margin-top:12px;max-width:100%}summary{cursor:pointer;font-weight:650}@media(max-width:760px){.shell{display:block}.side{padding:14px}.brand,.version{display:inline-block;margin:0 8px 10px 0}.nav{display:flex;overflow-x:auto;gap:3px}.nav a{white-space:nowrap}.main{padding:18px 13px}.top{display:block}.kv{grid-template-columns:minmax(0,1fr)}.kv span:nth-child(even){margin-bottom:7px}.card{padding:15px}}
)CSS";

struct TimezoneOption { const char* label; const char* value; };
const TimezoneOption TimezoneOptions[] = {
    {"Europe/Berlin", "CET-1CEST,M3.5.0/2,M10.5.0/3"}, {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"UTC", "UTC0"}, {"America/New_York", "EST5EDT,M3.2.0/2,M11.1.0/2"},
    {"America/Chicago", "CST6CDT,M3.2.0/2,M11.1.0/2"}, {"America/Denver", "MST7MDT,M3.2.0/2,M11.1.0/2"},
    {"America/Los_Angeles", "PST8PDT,M3.2.0/2,M11.1.0/2"},
};

String escapeHtml(const String& value) {
    String escaped;
    escaped.reserve(value.length() * 2 + 8);
    for (size_t index = 0; index < value.length(); ++index) {
        switch (value[index]) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += value[index]; break;
        }
    }
    return escaped;
}

String badge(const char* text, const char* style) {
    return "<span class='badge " + String(style) + "'>" + text + "</span>";
}

const char* sensorStateName(SensorState state) {
    switch (state) {
        case SensorState::Initializing: return "Initializing";
        case SensorState::Ready: return "Ready";
        case SensorState::Degraded: return "Degraded";
        case SensorState::Failed: return "Failed";
        default: return "Unknown";
    }
}

String lastMeasurementDisplay(
    const SensorRuntimeStatus& status,
    LocaleFormatter& localeFormatter) {
    if (!status.hasMeasurementActivity) return "—";

    String result;
    if (status.hasLastMeasurement) {
        tm localTime;
        const time_t timestamp = status.lastMeasurementEpoch;
        result = localtime_r(&timestamp, &localTime) != nullptr
            ? localeFormatter.formatDateTime(localTime)
            : String("—");
    } else {
        result = "Pre-sync activity";
    }

    const uint32_t ageSeconds = (millis() - status.lastMeasurementMonotonicMs) / 1000UL;
    result += "<br><span class='help'>";
    result += localeFormatter.formatNumber(ageSeconds, 0);
    result += " s ago</span>";
    return result;
}

String timezoneSelect(const String& current) {
    String html;
    html.reserve(720 + current.length() * 2);
    html = "<select name='timezone'>";
    bool found = false;
    for (const auto& option : TimezoneOptions) {
        const bool selected = current == option.value;
        found = found || selected;
        html += "<option value='" + escapeHtml(option.value) + "'" + (selected ? " selected" : "") + ">" + option.label + "</option>";
    }
    if (!found && !current.isEmpty()) html += "<option value='" + escapeHtml(current) + "' selected>Custom: " + escapeHtml(current) + "</option>";
    html += "</select>";
    return html;
}

String localeSelect(Locale current) {
    const Locale supported[] = {
        Locale::GermanGermany,
        Locale::EnglishUnitedKingdom,
        Locale::EnglishUnitedStates,
    };
    String html;
    html.reserve(220);
    html = "<select name='locale'>";
    for (const Locale locale : supported) {
        const char* key = localeKey(locale);
        html += "<option value='";
        html += key;
        html += "'";
        if (locale == current) html += " selected";
        html += ">";
        html += key;
        html += "</option>";
    }
    html += "</select>";
    return html;
}

String unitSelect(const char* name, MeasurementType type, PresentationUnit current) {
    const MeasurementTypeMetadata& metadata = measurementTypeMetadata(type);
    String html;
    html.reserve(64 + metadata.supportedPresentationUnitCount * 100);
    html = "<select name='";
    html += name;
    html += "'>";
    for (uint8_t index = 0; index < metadata.supportedPresentationUnitCount; ++index) {
        const PresentationUnit unit = metadata.supportedPresentationUnits[index];
        html += "<option value='" + String(UnitConverter::stableKey(unit)) + "'" + (unit == current ? " selected" : "") + ">";
        html += UnitConverter::displayName(unit);
        html += " (" + String(UnitConverter::symbol(unit)) + ")</option>";
    }
    html += "</select>";
    return html;
}

bool parseUnit(const String& value, MeasurementType type, PresentationUnit& unit) {
    return UnitConverter::parseStableKey(value.c_str(), unit) && supportsPresentationUnit(type, unit);
}

String measurementTypes(const SensorRuntimeInfo& info) {
    String result;
    result.reserve(180);
    const auto appendType = [&result](bool supported, MeasurementType type) {
        if (supported) result += String(measurementTypeMetadata(type).displayName) + ", ";
    };
    appendType(info.supportsTemperature, MeasurementType::Temperature);
    appendType(info.supportsRelativeHumidity, MeasurementType::RelativeHumidity);
    appendType(info.supportsAtmosphericPressure, MeasurementType::AtmosphericPressure);
    appendType(info.supportsSolarIrradiance, MeasurementType::SolarIrradiance);
    appendType(info.supportsSolarCellTemperature, MeasurementType::SolarCellTemperature);
    appendType(info.supportsRainDetectorLevel, MeasurementType::RainDetectorLevel);
    appendType(info.supportsRainDetectorWet, MeasurementType::RainDetectorWet);
    appendType(info.supportsRainGaugeTip, MeasurementType::RainGaugeTip);
    appendType(info.supportsRainfallIncrement, MeasurementType::RainfallIncrement);
    if (result.endsWith(", ")) result.remove(result.length() - 2);
    return result;
}

String hardwareAssignment(const SensorRuntimeInfo& info) {
    if (info.hardware.kind == HardwareResourceKind::GPIO) {
        return "GPIO" + String(info.hardware.gpio.number);
    }
    if (info.hardware.kind == HardwareResourceKind::I2C) {
        return String(i2cBusName(info.hardware.i2c.bus)) + " / 0x"
            + String(info.hardware.i2c.address, HEX);
    }
    return "None";
}

String configuredHardwareAssignment(const SensorSlotConfiguration& slot) {
    if (slot.hardware.kind == HardwareResourceKind::GPIO) {
        return "GPIO" + String(slot.hardware.gpio.number);
    }
    if (slot.hardware.kind == HardwareResourceKind::I2C) {
        return String(i2cBusName(slot.hardware.i2c.bus)) + " / 0x"
            + String(slot.hardware.i2c.address, HEX);
    }
    return "None";
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
    SensorId editedSlotId,
    GpioResource gpio) {
    const HardwareResourceAssignment candidate =
        HardwareResourceAssignment::gpioResource(gpio);
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        const SensorSlotConfiguration& other = configuration.sensorSlots[index];
        if (other.slotId == editedSlotId
            || !other.enabled
            || other.implementation == SensorImplementation::None) {
            continue;
        }
        if (exclusiveHardwareResourceConflict(other.hardware, candidate)) return true;
    }
    return false;
}

String implementationMeasurements(const SensorImplementationMetadata* metadata) {
    if (metadata == nullptr || metadata->measurementTypeCount == 0) return "None";
    String result;
    for (size_t index = 0; index < metadata->measurementTypeCount; ++index) {
        result += measurementTypeMetadata(metadata->measurementTypes[index]).displayName;
        if (index + 1 < metadata->measurementTypeCount) result += ", ";
    }
    return result;
}

String availableValue(const String& value) {
    return value.isEmpty() || value == "0.0.0.0" ? String("—") : escapeHtml(value);
}

String effectiveConnectionMode(bool setupAccessPoint) {
    if (setupAccessPoint) return String("—");
    esp_netif_t* stationInterface = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_dhcp_status_t dhcpStatus = ESP_NETIF_DHCP_INIT;
    if (stationInterface == nullptr
        || esp_netif_dhcpc_get_status(stationInterface, &dhcpStatus) != ESP_OK) {
        return String("—");
    }
    return dhcpStatus == ESP_NETIF_DHCP_STARTED ? String("DHCP")
        : dhcpStatus == ESP_NETIF_DHCP_STOPPED ? String("Static IP")
        : String("—");
}

bool runtimeSupportsMeasurement(const SensorRuntimeInfo& info, MeasurementType type) {
    switch (type) {
        case MeasurementType::Temperature: return info.supportsTemperature;
        case MeasurementType::RelativeHumidity: return info.supportsRelativeHumidity;
        case MeasurementType::AtmosphericPressure: return info.supportsAtmosphericPressure;
        case MeasurementType::SolarIrradiance: return info.supportsSolarIrradiance;
        case MeasurementType::SolarCellTemperature: return info.supportsSolarCellTemperature;
        case MeasurementType::RainDetectorLevel: return info.supportsRainDetectorLevel;
        case MeasurementType::RainDetectorWet: return info.supportsRainDetectorWet;
        case MeasurementType::RainGaugeTip: return info.supportsRainGaugeTip;
        case MeasurementType::RainfallIncrement: return info.supportsRainfallIncrement;
        default: return false;
    }
}

const char* measurementQualityName(MeasurementQuality quality) {
    switch (quality) {
        case MeasurementQuality::Good: return "Good";
        case MeasurementQuality::Estimated: return "Estimated";
        case MeasurementQuality::Degraded: return "Degraded";
        default: return "—";
    }
}

String presentedMeasurementValue(
    const Measurement& measurement,
    const Configuration& configuration,
    LocaleFormatter& localeFormatter) {
    if (!measurement.valid) return "Invalid";
    const MeasurementTypeMetadata& metadata = measurementTypeMetadata(measurement.type);
    switch (measurement.value.kind()) {
        case ValueKind::FloatingPoint: {
            float canonicalValue = 0.0F;
            if (!measurement.value.tryGetFloatingPoint(canonicalValue)) return "Invalid";
            PresentationUnit unit = configuration.presentationUnitFor(measurement.type);
            if (!supportsPresentationUnit(measurement.type, unit)) unit = metadata.defaultPresentationUnit;
            float presentedValue = 0.0F;
            if (!UnitConverter::convert(measurement.type, canonicalValue, unit, presentedValue)) {
                return "Invalid";
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
                ? String(value ? "True" : "False") : String("Invalid");
        }
        case ValueKind::UnsignedInteger: {
            uint32_t value = 0;
            return measurement.value.tryGetUnsignedInteger(value)
                ? localeFormatter.formatNumber(value, 0) : String("Invalid");
        }
        case ValueKind::None:
            return metadata.semantics == MeasurementSemantics::Event ? String("Event") : String("—");
        default:
            return "Invalid";
    }
}

String measurementTimeDisplay(
    const MeasurementSnapshot& snapshot,
    LocaleFormatter& localeFormatter) {
    tm localTime;
    const time_t timestamp = snapshot.measurement.timestamp;
    if (localtime_r(&timestamp, &localTime) == nullptr) return "—";
    String result = localeFormatter.formatDateTime(localTime);
    result += "<br><span class='help'>";
    result += localeFormatter.formatNumber(
        (millis() - snapshot.acceptedMonotonicMs) / 1000UL, 0);
    result += " s ago</span>";
    return result;
}

} // namespace

WebService::WebService(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService,
    IMqttService& mqttService, ITimeService& timeService, LocaleFormatter& localeFormatter,
    SensorManager& sensorManager, MeasurementSnapshotCache& measurementSnapshotCache,
    IDiscoveryPublisher& discoveryPublisher, RuntimeManager& runtimeManager, OTAService& otaService)
    : logger_(logger), configurationService_(configurationService), wifiService_(wifiService),
      mqttService_(mqttService), timeService_(timeService), localeFormatter_(localeFormatter),
      sensorManager_(sensorManager), measurementSnapshotCache_(measurementSnapshotCache),
      discoveryPublisher_(discoveryPublisher), runtimeManager_(runtimeManager), otaService_(otaService) {}

void WebService::begin() {
    server_.on("/", HTTP_GET, [this]() { handleStatus(); });
    server_.on("/status", HTTP_GET, [this]() { handleStatus(); });
    server_.on("/sensors", HTTP_GET, [this]() { handleSensors(); });
    server_.on("/measurements", HTTP_GET, [this]() { handleMeasurements(); });
    server_.on("/sensors/edit", HTTP_GET, [this]() { handleSensorEdit(); });
    server_.on("/network", HTTP_GET, [this]() { handleNetwork(); });
    server_.on("/mqtt", HTTP_GET, [this]() { handleMqtt(); });
    server_.on("/time", HTTP_GET, [this]() { handleTime(); });
    server_.on("/units", HTTP_GET, [this]() { handleUnits(); });
    server_.on("/device", HTTP_GET, [this]() { handleDevice(); });
    server_.on("/diagnostics", HTTP_GET, [this]() { handleDiagnostics(); });
    server_.on("/firmware", HTTP_GET, [this]() { handleFirmware(); });
    server_.on("/firmware/upload", HTTP_POST,
        [this]() { handleFirmwareUpload(); },
        [this]() { handleFirmwareUploadData(); });
    server_.on("/style.css", HTTP_GET, [this]() { handleStyle(); });
    server_.on("/network/save", HTTP_POST, [this]() { handleNetworkSave(); });
    server_.on("/mqtt/save", HTTP_POST, [this]() { handleMqttSave(); });
    server_.on("/mqtt/discovery/republish", HTTP_POST,
        [this]() { handleDiscoveryRepublish(); });
    server_.on("/time/save", HTTP_POST, [this]() { handleTimeSave(); });
    server_.on("/units/save", HTTP_POST, [this]() { handleUnitsSave(); });
    server_.on("/device/save", HTTP_POST, [this]() { handleDeviceSave(); });
    server_.on("/sensors/save", HTTP_POST, [this]() { handleSensorSave(); });
    server_.on("/sensors/apply", HTTP_POST, [this]() { handleSensorApply(); });
    server_.on("/restart", HTTP_POST, [this]() { handleRestart(); });
    server_.on("/factory-reset", HTTP_POST, [this]() { handleFactoryReset(); });
    server_.onNotFound([this]() { handleNotFound(); });
    server_.begin();
    logger_.println("Web administration started");
}

void WebService::loop() {
    server_.handleClient();
}

bool WebService::administrationAvailable() const { return wifiService_.inSetupAccessPointMode() || wifiService_.connected(); }

void WebService::sendPage(const char* title, const char* activeRoute, const String& content, int status) {
    if (!administrationAvailable()) {
        server_.send(503, "text/plain", "Administration unavailable while network is connecting");
        return;
    }
    const String page = renderPage(title, activeRoute, content);
    server_.send(status, "text/html; charset=utf-8", page);
}

void WebService::sendResult(const char* title, const char* route, const char* message, bool success) {
    String content;
    content.reserve(240 + strlen(title) + strlen(message) + strlen(route));
    content = "<div class='notice ";
    content += success ? "success" : "error";
    content += "'><strong>";
    content += escapeHtml(title);
    content += "</strong><p>";
    content += escapeHtml(message);
    content += "</p></div><a class='button' href='";
    content += route;
    content += "'>Back</a>";
    sendPage(title, route, content, success ? 200 : 400);
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
        case RuntimeAction::None: return "Configuration saved. Changes are active.";
        case RuntimeAction::RestartMqtt: return "Configuration saved. MQTT restart required.";
        case RuntimeAction::RestartTime: return "Configuration saved. Time service restart required.";
        case RuntimeAction::RestartWiFi: return "Configuration saved. WiFi restart required.";
        case RuntimeAction::RestartSensorManager: return "Configuration saved. Sensor Manager restart required.";
        case RuntimeAction::RestartDevice: return "Configuration saved. Device restart required.";
        default: return "Configuration saved. Runtime action required.";
    }
}

String WebService::pendingRuntimeActionHtml() const {
    const RuntimeAction action = runtimeManager_.pendingAction();
    if (action == RuntimeAction::None) return String();
    String html;
    html.reserve(180);
    html = "<div class='notice'><strong>Restart required</strong><p>Pending runtime action: ";
    switch (action) {
        case RuntimeAction::RestartMqtt: html += "MQTT restart"; break;
        case RuntimeAction::RestartTime: html += "Time service restart"; break;
        case RuntimeAction::RestartWiFi: html += "WiFi restart"; break;
        case RuntimeAction::RestartSensorManager: html += "Sensor Manager restart"; break;
        case RuntimeAction::RestartDevice: html += "Device restart"; break;
        case RuntimeAction::None: break;
    }
    html += ".</p></div>";
    return html;
}

String WebService::otaStatusHtml() const {
    String html;
    html.reserve(520);
    html = "<div class='kv'><span>OTA state</span><span>";
    html += otaStateName(otaService_.state());
    html += "</span><span>Upload status</span><span>";
    if (otaService_.totalBytesKnown()) {
        html += localeFormatter_.formatNumber(otaService_.bytesReceived(), 0);
        html += " / ";
        html += localeFormatter_.formatNumber(otaService_.totalBytes(), 0);
        html += " bytes (";
        html += localeFormatter_.formatNumber(otaService_.progressPercent(), 0);
        html += "%)";
    } else if (otaService_.bytesReceived() > 0) {
        html += localeFormatter_.formatNumber(otaService_.bytesReceived(), 0);
        html += " bytes received";
    } else {
        html += "—";
    }
    html += "</span><span>Restart required</span><span>";
    html += otaService_.restartRequired() ? "Yes" : "No";
    html += "</span>";
    if (otaService_.state() == OTAState::Failed && !otaService_.lastError().isEmpty()) {
        html += "<span>Last error</span><span>";
        html += escapeHtml(otaService_.lastError());
        html += "</span>";
    }
    html += "</div>";
    return html;
}

String WebService::navigationHtml(const char* active) const {
    const char* routes[][2] = {{"/status","Status"},{"/sensors","Sensors"},{"/measurements","Measurements"},{"/network","Network"},{"/mqtt","MQTT"},{"/time","Locale & Time"},{"/units","Units"},{"/device","Device"},{"/diagnostics","Diagnostics"},{"/firmware","Firmware"}};
    String html;
    html.reserve(560);
    html = "<nav class='nav'>";
    for (const auto& route : routes) {
        html += "<a href='";
        html += route[0];
        html += "' class='";
        if (strcmp(active, route[0]) == 0) html += "active";
        html += "'>";
        html += route[1];
        html += "</a>";
    }
    html += "</nav>";
    return html;
}

String WebService::renderPage(const char* title, const char* active, const String& content) const {
    const Configuration& cfg = configurationService_.getConfiguration();
    String html;
    html.reserve(content.length() + 1200);
    html = "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>";
    html += escapeHtml(title);
    html += " · WeatherStation</title><link rel='stylesheet' href='/style.css'></head><body><div class='shell'><aside class='side'><div class='brand'>";
    html += escapeHtml(cfg.device.name);
    html += "</div><div class='version'>WeatherStation · v";
    html += FirmwareBuildInfo::SemanticVersion;
    html += "</div>";
    html += navigationHtml(active);
    html += "</aside><main class='main'><div class='top'><div><h1>";
    html += escapeHtml(title);
    html += "</h1>";
    if (wifiService_.inSetupAccessPointMode()) html += "<p class='muted'>Setup access point mode</p>";
    html += "</div></div>" + pendingRuntimeActionHtml() + content + "</main></div></body></html>";
    return html;
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
    c = "<div class='grid'><section class='card'><h2>Device</h2><div class='kv'><span>Name</span><span>" + escapeHtml(cfg.device.name) + "</span><span>Firmware</span><span>" + FirmwareBuildInfo::SemanticVersion + "</span><span>Build</span><span>" + FirmwareBuildInfo::CompactIdentity + "</span><span>Uptime</span><span>" + localeFormatter_.formatNumber(millis()/1000UL, 0) + " seconds</span><span>Free heap</span><span>" + localeFormatter_.formatNumber(ESP.getFreeHeap(), 0) + " bytes</span><span>Flash</span><span>" + localeFormatter_.formatNumber(ESP.getFlashChipSize()/1024UL, 0) + " KB</span></div></section>";
    c += "<section class='card'><h2>Network</h2><div class='kv'><span>Status</span><span>" + (connected?badge("Connected","good"):setupAccessPoint?badge("Setup AP","warn"):badge("Disconnected","bad")) + "</span>";
    c += "<span>Connection mode</span><span>" + effectiveConnectionMode(setupAccessPoint) + "</span><span>Hostname</span><span>" + availableValue(hostname) + "</span><span>SSID</span><span>" + availableValue(ssid) + "</span>";
    c += "<span>IPv4 address</span><span>" + availableValue(ipv4Address) + "</span><span>Subnet mask</span><span>" + availableValue(subnetMask) + "</span><span>Default gateway</span><span>" + availableValue(gateway) + "</span>";
    c += "<span>DNS server 1</span><span>" + availableValue(dns1) + "</span>";
    if (!dns2.isEmpty() && dns2 != "0.0.0.0") c += "<span>DNS server 2</span><span>" + escapeHtml(dns2) + "</span>";
    c += "<span>MAC address</span><span>" + availableValue(macAddress) + "</span><span>RSSI</span><span>" + (connected?localeFormatter_.formatNumber(wifiService_.rssi(),0)+" dBm":"—") + "</span></div></section>";
    const bool configured = !cfg.mqtt.server.isEmpty();
    c += "<section class='card'><h2>MQTT</h2><div class='kv'><span>Configuration</span><span>" + badge(configured?"Configured":"Not configured",configured?"good":"warn") + "</span><span>Runtime</span><span>" + badge(mqttService_.connected()?"Connected":"Disconnected",mqttService_.connected()?"good":"bad") + "</span></div></section>";
    c += "<section class='card'><h2>Time</h2><div class='kv'><span>Status</span><span>" + badge(timeService_.synchronized()?"Synchronized":"Synchronizing",timeService_.synchronized()?"good":"warn") + "</span><span>Local time</span><span>" + (timeService_.synchronized()?escapeHtml(currentLocalDateTime()):"—") + "</span></div></section>";
    c += "<section class='card'><h2>Sensors</h2><div class='kv'><span>Registered</span><span>" + localeFormatter_.formatNumber(sensorManager_.sensorCount(), 0) + "</span></div><p><a href='/sensors'>View sensor runtime state</a></p></section></div>";
    sendPage("Status", "/status", c);
}

void WebService::handleNetwork() {
    const NetworkConfiguration& n = configurationService_.getConfiguration().network;
    const bool isStatic = n.addressMode == NetworkAddressMode::Static;
    String c;
    c.reserve(2200);
    c = "<section class='card'><h2>Network configuration</h2><form method='post' action='/network/save' autocomplete='off'>";
    c += "<label>Hostname<input name='hostname' required value='" + escapeHtml(n.hostname) + "'></label><label>WiFi SSID<input name='wifiSSID' required value='" + escapeHtml(n.wifiSSID) + "'></label>";
    c += "<label class='choice'><input type='checkbox' name='changeWifiPassword' value='1'>Change WiFi password</label><label>New WiFi password<input type='password' name='wifiPassword' autocomplete='new-password'></label>";
    c += "<label>Address mode<select name='addressMode' id='addressMode' onchange='toggleStatic()'><option value='dhcp'" + String(!isStatic?" selected":"") + ">DHCP</option><option value='static'" + String(isStatic?" selected":"") + ">Static IPv4</option></select></label>";
    c += "<div id='staticFields'><label>IPv4 address<input name='ipv4Address' value='" + escapeHtml(n.ipv4Address) + "'></label><label>Subnet mask<input name='subnetMask' value='" + escapeHtml(n.subnetMask) + "'></label><label>Default gateway<input name='gateway' value='" + escapeHtml(n.gateway) + "'></label><label>Primary DNS<input name='dns1' value='" + escapeHtml(n.dns1) + "'></label><label>Secondary DNS <span class='help'>(optional)</span><input name='dns2' value='" + escapeHtml(n.dns2) + "'></label></div><div class='actions'><button>Save network settings</button></div></form></section>";
    c += "<script>function toggleStatic(){var s=document.getElementById('addressMode').value==='static',d=document.getElementById('staticFields');d.hidden=!s;d.querySelectorAll('input').forEach(function(i){i.disabled=!s})}toggleStatic()</script>";
    sendPage("Network", "/network", c);
}

void WebService::handleMqtt() {
    const MqttConfiguration& m = configurationService_.getConfiguration().mqtt;
    String c;
    c.reserve(1300);
    c = "<section class='card'><h2>Runtime status</h2>" + (m.server.isEmpty()?badge("Not configured","warn"):mqttService_.connected()?badge("Connected","good"):badge("Disconnected","bad")) + "</section><section class='card'><h2>Broker configuration</h2><form method='post' action='/mqtt/save' autocomplete='off'>";
    c += "<label>Broker hostname or IP<input name='server' value='" + escapeHtml(m.server) + "'></label><label>Port<input type='number' min='1' max='65535' name='port' required value='" + String(m.port) + "'></label><label>Username<input name='username' value='" + escapeHtml(m.username) + "'></label><label class='choice'><input type='checkbox' name='changePassword' value='1'>Change MQTT password</label><label>New password<input type='password' name='password' autocomplete='new-password'></label><div class='actions'><button>Save MQTT settings</button></div></form></section>";
    c += "<section class='card'><h2>Home Assistant Discovery</h2><p class='help'>Republish the current retained Device Discovery configuration.</p><form method='post' action='/mqtt/discovery/republish'><button type='submit'>Republish Home Assistant Discovery</button></form></section>";
    sendPage("MQTT", "/mqtt", c);
}

void WebService::handleTime() {
    const TimeConfiguration& t = configurationService_.getConfiguration().time;
    const Locale currentLocale = configurationService_.getLocale();
    String c;
    c.reserve(1300);
    c = "<section class='card'><h2>Runtime status</h2><div class='kv'><span>Synchronization</span><span>" + badge(timeService_.synchronized()?"Synchronized":"Synchronizing",timeService_.synchronized()?"good":"warn") + "</span><span>Local time</span><span>" + (timeService_.synchronized()?escapeHtml(currentLocalDateTime()):"—") + "</span></div></section><form method='post' action='/time/save'><section class='card'><h2>Locale</h2><label>Locale" + localeSelect(currentLocale) + "</label></section><section class='card'><h2>Time</h2>";
    c += "<label>Timezone" + timezoneSelect(t.timezone) + "</label><label>NTP server 1<input name='ntpServer1' value='" + escapeHtml(t.ntpServer1) + "'></label><label>NTP server 2<input name='ntpServer2' value='" + escapeHtml(t.ntpServer2) + "'></label><div class='actions'><button>Save locale and time</button></div></section></form>";
    sendPage("Locale & Time", "/time", c);
}

void WebService::handleUnits() {
    const PresentationConfiguration& p = configurationService_.getConfiguration().presentation;
    String c;
    c.reserve(1400);
    c = "<section class='card'><h2>Presentation units</h2><p class='help'>Changes affect future MQTT payloads and do not alter canonical sensor measurements.</p><form method='post' action='/units/save'>";
    c += "<label>Temperature" + unitSelect("temperature",MeasurementType::Temperature,p.temperature) + "</label><label>Atmospheric Pressure" + unitSelect("pressure",MeasurementType::AtmosphericPressure,p.atmosphericPressure) + "</label><label>Solar Cell Temperature" + unitSelect("solarTemperature",MeasurementType::SolarCellTemperature,p.solarCellTemperature) + "</label><label>Rain Detector Level" + unitSelect("rainLevel",MeasurementType::RainDetectorLevel,p.rainDetectorLevel) + "</label><div class='actions'><button>Save units</button></div></form></section>";
    sendPage("Units", "/units", c);
}

void WebService::handleDevice() {
    String c;
    c.reserve(500);
    c = "<section class='card'><h2>Device identity</h2><form method='post' action='/device/save'><label>Device name<input name='deviceName' required value='" + escapeHtml(configurationService_.getConfiguration().device.name) + "'></label><div class='actions'><button>Save device settings</button></div></form></section>";
    sendPage("Device", "/device", c);
}

void WebService::handleSensors() {
    String c;
    c.reserve(1200 + MaxSensorSlotCount * 400);
    if (runtimeManager_.pendingAction() == RuntimeAction::RestartSensorManager) {
        c = "<div class='notice'><strong>Sensor restart required</strong><p>Saved sensor configuration differs from the active runtime composition.</p><form method='post' action='/sensors/apply'><button>Apply Sensor Changes</button></form></div>";
    }
    c += "<section class='card'><h2>Sensor Slots</h2><p class='help'>Saving and runtime activation are separate actions. Enabled with None is valid and creates no runtime Sensor.</p><div class='scroll'><table><thead><tr><th class='sensor-technical'>Slot</th><th>Name</th><th>Configured</th><th class='sensor-technical'>Connection</th><th class='sensor-technical'>Schedule</th><th>Runtime</th><th class='sensor-technical'>State</th><th class='sensor-last-measurement'>Last Measurement</th><th class='sensor-actions'></th></tr></thead><tbody>";
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
        c += slot.enabled ? badge("Enabled", "good") : badge("Disabled", "warn");
        c += "<br>" + escapeHtml(metadata == nullptr ? "Invalid" : metadata->displayType);
        c += "</td><td class='sensor-technical'>" + configuredHardwareAssignment(slot) + "</td><td class='sensor-technical'>";
        c += slot.schedule.acquisitionMode == AcquisitionMode::Periodic
            ? String(slot.schedule.sampleIntervalMs) + " ms" : "Event only";
        c += "</td><td>";
        c += hasRuntime ? escapeHtml(runtime.type) + " / " + hardwareAssignment(runtime) : "No runtime Sensor";
        const bool expectsRuntime = slot.enabled && slot.implementation != SensorImplementation::None;
        const bool runtimeMatches = expectsRuntime == hasRuntime
            && (!hasRuntime || (runtime.implementation == slot.implementation
                && String(runtime.name) == slot.name
                && runtime.schedule.acquisitionMode == slot.schedule.acquisitionMode
                && runtime.schedule.sampleIntervalMs == slot.schedule.sampleIntervalMs
                && sameHardwareAssignment(runtime.hardware, slot.hardware)));
        if (!runtimeMatches) c += "<br>" + badge("Sensor restart required", "warn");
        c += "</td><td class='sensor-technical'>" + String(hasRuntime ? sensorStateName(runtime.state) : "—") + "</td><td class='sensor-last-measurement'>";
        SensorRuntimeStatus runtimeStatus;
        c += hasRuntime && sensorManager_.runtimeStatus(runtime.id, runtimeStatus)
            ? lastMeasurementDisplay(runtimeStatus, localeFormatter_)
            : String("—");
        c += "</td>";
        c += "<td class='sensor-actions'><a class='button' href='/sensors/edit?slot=" + String(slot.slotId) + "'>Configure</a></td></tr>";
    }
    c += "</tbody></table></div></section>";
    sendPage("Sensors", "/sensors", c);
}

void WebService::handleMeasurements() {
    String content;
    content.reserve(600 + sensorManager_.sensorCount() * 1200);
    content = "<p class='help'>Current runtime snapshots only. Measurements are not stored as history.</p>";
    const Configuration& configuration = configurationService_.getConfiguration();
    for (size_t sensorIndex = 0; sensorIndex < sensorManager_.sensorCount(); ++sensorIndex) {
        SensorRuntimeInfo runtime;
        if (!sensorManager_.runtimeInfo(sensorIndex, runtime)) continue;
        content += "<section class='card'><h2>Slot ";
        content += localeFormatter_.formatNumber(runtime.id, 0);
        content += " · ";
        content += escapeHtml(runtime.name);
        content += "</h2><p class='help'>";
        content += escapeHtml(runtime.type);
        content += "</p><div class='scroll'><table><thead><tr><th>Measurement</th><th>Current Value</th><th>Quality</th><th class='sensor-last-measurement'>Last Accepted</th></tr></thead><tbody>";
        for (uint8_t typeValue = 1; typeValue <= SupportedMeasurementTypeCount; ++typeValue) {
            const MeasurementType type = static_cast<MeasurementType>(typeValue);
            if (!runtimeSupportsMeasurement(runtime, type)) continue;
            MeasurementSnapshot snapshot;
            const bool hasSnapshot = measurementSnapshotCache_.snapshot(runtime.id, type, snapshot);
            content += "<tr><td>";
            content += measurementTypeMetadata(type).displayName;
            content += "</td><td class='sensor-technical'>";
            content += hasSnapshot
                ? presentedMeasurementValue(snapshot.measurement, configuration, localeFormatter_)
                : String("—");
            content += "</td><td class='sensor-technical'>";
            content += hasSnapshot ? measurementQualityName(snapshot.measurement.quality) : "—";
            content += "</td><td class='sensor-last-measurement'>";
            content += hasSnapshot ? measurementTimeDisplay(snapshot, localeFormatter_) : String("—");
            content += "</td></tr>";
        }
        content += "</tbody></table></div></section>";
    }
    if (sensorManager_.sensorCount() == 0) {
        content += "<section class='card'><p>No active runtime Sensors.</p></section>";
    }
    sendPage("Measurements", "/measurements", content);
}

void WebService::handleSensorEdit() {
    const long requestedSlot = server_.arg("slot").toInt();
    if (requestedSlot < 1 || requestedSlot > static_cast<long>(MaxSensorSlotCount)) {
        sendResult("Invalid Sensor Slot", "/sensors", "The requested Slot does not exist.", false);
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
            + "' data-interval='" + String(metadata->defaultSchedule.sampleIntervalMs) + "'";
        if (metadata->implementation == slot.implementation) options += " selected";
        options += ">" + escapeHtml(metadata->displayType) + "</option>";
    }
    String gpioOptions;
    String i2cBusOptions;
    const BoardCapabilities& board = BoardCapabilities::current();
    const Configuration& configuration = configurationService_.getConfiguration();
    for (size_t index = 0; index < board.gpioCount(); ++index) {
        const BoardGpioCapability* gpio = board.gpioAt(index);
        if (gpio == nullptr) continue;
        const HardwareResourceAssignment candidate =
            HardwareResourceAssignment::gpioResource(gpio->resource);
        if (selected == nullptr
            || board.validate(selected->interfaceKind, candidate,
                selected->requiredGpioCapabilities)
                != HardwareResourceValidationResult::Valid
            || gpioAssignedToOtherEnabledSlot(configuration, slot.slotId, gpio->resource)) {
            continue;
        }
        gpioOptions += "<option value='" + String(gpio->resource.number) + "'";
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
    c = "<section class='card'><h2>Configure Slot " + String(slot.slotId) + "</h2><form method='post' action='/sensors/save'><input type='hidden' name='slot' value='" + String(slot.slotId) + "'>";
    c += "<label class='choice'><input type='checkbox' name='enabled' value='1'" + String(slot.enabled ? " checked" : "") + ">Enabled</label>";
    c += "<label>Name<input name='name' maxlength='" + String(MaxSensorSlotNameLength) + "' required value='" + escapeHtml(slot.name) + "'></label>";
    c += "<label>Implementation<select id='sensorImplementation' name='implementation'>" + options + "</select></label>";
    c += "<div id='gpioConfiguration'><label>GPIO<select name='gpio'>" + gpioOptions + "</select></label><p class='help'>AM2302 uses a custom single-wire protocol. Rain Gauge uses a digital interrupt.</p></div>";
    c += "<div id='i2cConfiguration'><label>Bus<select name='i2cBus'>" + i2cBusOptions + "</select></label><label>I²C address<select id='i2cAddress' name='i2cAddress'><option value='68'" + String(slot.hardware.kind == HardwareResourceKind::I2C && slot.hardware.i2c.address == 0x44 ? " selected" : "") + ">0x44</option><option value='118'" + String(slot.hardware.kind == HardwareResourceKind::I2C && slot.hardware.i2c.address == 0x76 ? " selected" : "") + ">0x76</option><option value='119'" + String(slot.hardware.kind == HardwareResourceKind::I2C && slot.hardware.i2c.address == 0x77 ? " selected" : "") + ">0x77</option></select></label></div>";
    c += "<div id='rainGaugeConfiguration'><label>Millimetres per tip<input type='number' name='millimetersPerTip' min='0.0001' max='100' step='0.0001' value='" + String(slot.implementationConfiguration.rainGauge.millimetersPerTip, 4) + "'></label><label>Debounce time (ms)<input type='number' name='debounceMs' min='1' max='5000' value='" + String(slot.implementationConfiguration.rainGauge.debounceMs) + "'></label></div>";
    c += "<label>Sample interval (ms)<input id='sensorInterval' type='number' min='1' max='2147483647' name='interval' value='" + String(slot.schedule.sampleIntervalMs) + "'></label>";
    c += "<div class='actions'><button type='submit'>Save Slot</button><a class='button' href='/sensors'>Cancel</a></div></form></section>";
    if (selected != nullptr) {
        c += "<section class='card'><h2>Implementation metadata</h2><div class='kv'><span>Type</span><span>" + escapeHtml(selected->displayType) + "</span><span>Interface</span><span>" + hardwareInterfaceKindName(selected->interfaceKind) + " / " + escapeHtml(selected->protocolDescription) + "</span><span>Provenance</span><span>" + String(selected->provenance == SensorProvenance::Simulated ? "Simulated" : "Physical") + "</span><span>Measurements</span><span>" + implementationMeasurements(selected) + "</span></div></section>";
    }
    c += "<script>function sensorFields(reset){const s=document.getElementById('sensorImplementation');const o=s.options[s.selectedIndex];document.getElementById('gpioConfiguration').style.display=o.dataset.interface==='GPIO'?'block':'none';document.getElementById('i2cConfiguration').style.display=o.dataset.interface==='I2C'?'block':'none';document.getElementById('rainGaugeConfiguration').style.display=o.dataset.kind==='rain_gauge'?'block':'none';const a=document.getElementById('i2cAddress');for(const x of a.options)x.hidden=o.dataset.kind==='sht4x'?x.value!=='68':o.dataset.kind==='bme280'?x.value==='68':false;if(reset&&o.dataset.kind==='sht4x')a.value='68';if(reset&&o.dataset.kind==='bme280'&&a.value==='68')a.value='118';const n=Number(o.dataset.interval);const f=document.getElementById('sensorInterval');f.parentElement.style.display=n>0?'block':'none';f.disabled=n<=0;if(reset)f.value=n}document.getElementById('sensorImplementation').addEventListener('change',()=>sensorFields(true));sensorFields(false);</script>";
    sendPage("Configure Sensor Slot", "/sensors", c);
}

void WebService::handleDiagnostics() {
    String c;
    c.reserve(750 + sensorManager_.sensorCount() * 450);
    c = "<div class='grid'><section class='card'><h2>System</h2><div class='kv'><span>Uptime</span><span>"+localeFormatter_.formatNumber(millis()/1000UL,0)+" s</span><span>Free heap</span><span>"+localeFormatter_.formatNumber(ESP.getFreeHeap(),0)+" bytes</span><span>Flash</span><span>"+localeFormatter_.formatNumber(ESP.getFlashChipSize(),0)+" bytes</span></div></section><section class='card'><h2>Services</h2><div class='kv'><span>WiFi</span><span>"+(wifiService_.connected()?"Connected":"Disconnected")+"</span><span>MQTT</span><span>"+(mqttService_.connected()?"Connected":"Disconnected")+"</span><span>Time</span><span>"+(timeService_.synchronized()?"Synchronized":"Pending")+"</span></div></section></div>";
    for(size_t index=0;index<sensorManager_.sensorCount();++index){SensorRuntimeInfo i; if(!sensorManager_.runtimeInfo(index,i))continue;SensorRuntimeStatus s;sensorManager_.runtimeStatus(i.id,s);c+="<section class='card'><h2>Sensor "+localeFormatter_.formatNumber(i.id,0)+" · "+escapeHtml(i.name)+"</h2><div class='kv'><span>Type</span><span>"+escapeHtml(i.type)+"</span><span>State</span><span>"+sensorStateName(i.state)+"</span><span>Accepted</span><span>"+localeFormatter_.formatNumber(s.acceptedMeasurementCount,0)+"</span><span>Rejected</span><span>"+localeFormatter_.formatNumber(s.rejectedMeasurementCount,0)+"</span><span>Pre-sync discarded</span><span>"+localeFormatter_.formatNumber(s.preSyncDiscardCount,0)+"</span><span>Last sample emissions</span><span>"+localeFormatter_.formatNumber(s.lastSampleEmissionCount,0)+"</span></div></section>";}
    sendPage("Diagnostics", "/diagnostics", c);
}

void WebService::handleFirmware() {
    String c;
    c.reserve(2000);
    const char* sourceStyle = FirmwareBuildInfo::SourceDirty ? "warn"
        : String(FirmwareBuildInfo::SourceState) == "clean" ? "good" : "warn";
    c = "<section class='card'><h2>Running firmware</h2><div class='kv'><span>Firmware Version</span><span>" + String(FirmwareBuildInfo::SemanticVersion) + "</span><span>Build Identity</span><span>" + FirmwareBuildInfo::CompactIdentity + "</span><span>Build Number</span><span>" + FirmwareBuildInfo::BuildNumber + "</span><span>Git Commit</span><span>" + FirmwareBuildInfo::GitCommit + "</span><span>Git Branch</span><span>" + escapeHtml(FirmwareBuildInfo::GitBranch) + "</span><span>Build Timestamp</span><span>" + FirmwareBuildInfo::BuildTimestampUtc + "</span><span>Source State</span><span>" + badge(FirmwareBuildInfo::SourceDirty ? "Dirty" : String(FirmwareBuildInfo::SourceState) == "clean" ? "Clean" : "Unknown", sourceStyle) + "</span><span>Staged firmware</span><span>"+(otaService_.firmwareStaged()?"Ready for activation":"—")+"</span></div>";
    if (otaService_.firmwareStaged()) c += "<p class='help'>Staged firmware version metadata is not yet available.</p>";
    c += "</section><section class='card'><h2>Firmware update status</h2>" + otaStatusHtml() + "</section>";
    if (!otaService_.firmwareStaged() && !otaService_.busy()) {
        c += "<section class='card'><h2>Upload firmware</h2><p class='help'>Build with <code>pio run</code>, then upload <code>.pio/build/&lt;environment&gt;/firmware.bin</code>.</p><form method='post' action='/firmware/upload' enctype='multipart/form-data' onsubmit='document.getElementById(\"firmwareSize\").value=document.getElementById(\"firmwareFile\").files[0].size'><input type='hidden' id='firmwareSize' name='firmwareSize' value='0'><label>Firmware binary<input id='firmwareFile' type='file' name='firmware' accept='.bin,application/octet-stream' required></label><div class='actions'><button>Upload firmware</button></div></form></section>";
    }
    c += "<section class='card'><h2>Restart</h2><form method='post' action='/restart' onsubmit='return confirm(\"Restart the device now?\")'><button>Restart Now</button></form></section><section class='card'><h2>Factory reset</h2><p>This permanently removes all stored configuration and restarts into provisioning mode.</p><form method='post' action='/factory-reset' onsubmit='return confirm(\"Erase ALL configuration and restart?\")'><button class='danger'>Factory reset</button></form></section>";
    sendPage("Firmware", "/firmware", c);
}

void WebService::handleFirmwareUploadData() {
    HTTPUpload& upload = server_.upload();
    if (upload.status == UPLOAD_FILE_START) {
        firmwareUploadRequestAccepted_ = false;
        firmwareUploadRequestError_ = String();
        if (otaService_.busy()) {
            firmwareUploadRequestError_ = "Another firmware upload is already active.";
            return;
        }
        if (otaService_.firmwareStaged()) {
            firmwareUploadRequestError_ = "Firmware is already staged and awaiting restart.";
            return;
        }
        if (upload.name != "firmware" || upload.filename.isEmpty()
            || !upload.filename.endsWith(".bin")) {
            firmwareUploadRequestError_ = "Select a valid PlatformIO firmware.bin file.";
            otaService_.rejectUpload(firmwareUploadRequestError_.c_str());
            return;
        }
        const size_t expectedSize = static_cast<size_t>(server_.arg("firmwareSize").toInt());
        firmwareUploadRequestAccepted_ = otaService_.beginUpload(expectedSize);
        if (!firmwareUploadRequestAccepted_) {
            firmwareUploadRequestError_ = otaService_.lastError().isEmpty()
                ? String("Firmware upload could not be started.") : otaService_.lastError();
        }
        return;
    }
    if (!firmwareUploadRequestAccepted_) return;
    if (upload.status == UPLOAD_FILE_WRITE) {
        if (!otaService_.writeChunk(upload.buf, upload.currentSize)) {
            firmwareUploadRequestAccepted_ = false;
            firmwareUploadRequestError_ = otaService_.lastError();
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (!otaService_.finishUpload()) {
            firmwareUploadRequestAccepted_ = false;
            firmwareUploadRequestError_ = otaService_.lastError();
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        otaService_.abortUpload("Firmware upload was aborted before completion.");
        firmwareUploadRequestAccepted_ = false;
        firmwareUploadRequestError_ = otaService_.lastError();
    }
}

void WebService::handleFirmwareUpload() {
    if (firmwareUploadRequestAccepted_ && otaService_.firmwareStaged()) {
        String content;
        content.reserve(520);
        content = "<div class='notice success'><strong>Firmware uploaded successfully.</strong><p>Firmware is staged.</p><p>Device restart is required to activate the new firmware.</p></div><form method='post' action='/restart' onsubmit='return confirm(\"Restart the device now?\")'><button>Restart Now</button></form><a class='button' href='/firmware'>Restart Later</a>";
        sendPage("Firmware staged", "/firmware", content);
        return;
    }
    const String reason = !firmwareUploadRequestError_.isEmpty()
        ? firmwareUploadRequestError_ : otaService_.lastError();
    String content;
    content.reserve(480 + reason.length());
    content = "<div class='notice error'><strong>Firmware update failed.</strong><p>";
    content += escapeHtml(reason.isEmpty() ? String("Invalid firmware upload request.") : reason);
    content += "</p><p>The currently running firmware remains active.</p></div><a class='button' href='/firmware'>Back</a>";
    sendPage("Firmware update failed", "/firmware", content, 400);
}

void WebService::handleStyle() {
    const size_t styleLength = strlen_P(SharedStyle);
    server_.sendHeader("Cache-Control", "public, max-age=86400");
    server_.send_P(200, PSTR("text/css"), SharedStyle, styleLength);
}

void WebService::handleNetworkSave() {
    NetworkConfiguration n = configurationService_.getConfiguration().network;
    n.hostname=server_.arg("hostname"); n.wifiSSID=server_.arg("wifiSSID"); n.addressMode=server_.arg("addressMode")=="static"?NetworkAddressMode::Static:NetworkAddressMode::Dhcp;
    n.ipv4Address=server_.arg("ipv4Address"); n.subnetMask=server_.arg("subnetMask"); n.gateway=server_.arg("gateway"); n.dns1=server_.arg("dns1"); n.dns2=server_.arg("dns2");
    const bool updatePassword=server_.arg("changeWifiPassword")=="1"; if(updatePassword)n.wifiPassword=server_.arg("wifiPassword");
    const ConfigurationSaveResult result=configurationSaveResult(configurationService_.setNetworkConfiguration(n,updatePassword),ConfigurationArea::Network);
    sendConfigurationResult(result,"Network settings saved","Network save failed","/network","Invalid network settings. Static mode requires a valid address, contiguous subnet mask, same-subnet gateway, and primary DNS.");
}

void WebService::handleMqttSave() {
    const String portText=server_.arg("port"); const long port=portText.toInt();
    bool ok=port>=1&&port<=65535&&configurationService_.setMqttServer(server_.arg("server"))&&configurationService_.setMqttPort(static_cast<uint16_t>(port))&&configurationService_.setMqttUsername(server_.arg("username"));
    if(ok&&server_.arg("changePassword")=="1")ok=configurationService_.setMqttPassword(server_.arg("password"));
    sendConfigurationResult(configurationSaveResult(ok,ConfigurationArea::Mqtt),"MQTT settings saved","MQTT save failed","/mqtt","Invalid MQTT configuration.");
}

void WebService::handleDiscoveryRepublish() {
    switch (discoveryPublisher_.republish()) {
        case DiscoveryRepublishResult::Published:
            sendResult("Home Assistant Discovery published", "/mqtt",
                "Home Assistant Discovery published.", true);
            return;
        case DiscoveryRepublishResult::MqttUnavailable:
            sendResult("Home Assistant Discovery unavailable", "/mqtt",
                "MQTT is unavailable. Discovery was not published; retry when MQTT is connected.", false);
            return;
        case DiscoveryRepublishResult::PublishFailed:
        default:
            sendResult("Home Assistant Discovery failed", "/mqtt",
                "MQTT publication failed. Discovery was not updated.", false);
            return;
    }
}

void WebService::handleTimeSave() { const Configuration& current=configurationService_.getConfiguration(); Locale locale; bool ok=parseLocaleKey(server_.arg("locale").c_str(),locale); const bool localeChanged=ok&&locale!=current.locale.locale; const bool timeChanged=server_.arg("timezone")!=current.time.timezone||server_.arg("ntpServer1")!=current.time.ntpServer1||server_.arg("ntpServer2")!=current.time.ntpServer2; if(ok)ok=configurationService_.setLocale(locale)&&configurationService_.setTimezone(server_.arg("timezone"))&&configurationService_.setNtpServer1(server_.arg("ntpServer1"))&&configurationService_.setNtpServer2(server_.arg("ntpServer2")); sendConfigurationResult(configurationSaveResult(ok,ConfigurationArea::Locale,localeChanged,ConfigurationArea::Time,timeChanged),"Locale and time saved","Locale and time save failed","/time","Invalid locale or time configuration."); }

void WebService::handleUnitsSave() {
    PresentationUnit t,p,s,r; bool ok=parseUnit(server_.arg("temperature"),MeasurementType::Temperature,t)&&parseUnit(server_.arg("pressure"),MeasurementType::AtmosphericPressure,p)&&parseUnit(server_.arg("solarTemperature"),MeasurementType::SolarCellTemperature,s)&&parseUnit(server_.arg("rainLevel"),MeasurementType::RainDetectorLevel,r);
    if(ok)ok=configurationService_.setPresentationUnit(MeasurementType::Temperature,t)&&configurationService_.setPresentationUnit(MeasurementType::AtmosphericPressure,p)&&configurationService_.setPresentationUnit(MeasurementType::SolarCellTemperature,s)&&configurationService_.setPresentationUnit(MeasurementType::RainDetectorLevel,r);
    sendConfigurationResult(configurationSaveResult(ok,ConfigurationArea::PresentationUnits),"Presentation units saved","Unit save failed","/units","Invalid or unsupported presentation unit.");
}

void WebService::handleDeviceSave() { const bool ok=configurationService_.setDeviceName(server_.arg("deviceName")); sendConfigurationResult(configurationSaveResult(ok,ConfigurationArea::Device),"Device settings saved","Device save failed","/device","Invalid device name."); }
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
            || slot.implementation == SensorImplementation::SHT4x) {
            const long address = server_.arg("i2cAddress").toInt();
            const long busValue = server_.arg("i2cBus").toInt();
            const I2CBus bus = static_cast<I2CBus>(busValue);
            const bool validAddress = slot.implementation == SensorImplementation::SHT4x
                ? address == 0x44 : (address == 0x76 || address == 0x77);
            if (!validAddress
                || busValue < 0 || busValue > 255
                || BoardCapabilities::current().i2cBus(bus) == nullptr) {
                ok = false;
            } else {
                const I2CResource resource(bus, static_cast<uint8_t>(address));
                slot.hardware = HardwareResourceAssignment::i2cResource(resource);
                slot.implementationConfiguration.bme280 = BME280Configuration(resource);
                slot.implementationConfiguration.sht4x = SHT4xConfiguration(resource);
            }
        }
    }
    if (ok) ok = configurationService_.setSensorSlotConfiguration(slot);
    sendConfigurationResult(
        configurationSaveResult(ok, ConfigurationArea::Sensors),
        "Sensor Slot saved",
        "Sensor Slot save failed",
        "/sensors",
        "Invalid Slot configuration, schedule, hardware resource, or resource conflict.");
}
void WebService::handleSensorApply() {
    if (runtimeManager_.pendingAction() != RuntimeAction::RestartSensorManager) {
        sendResult(
            "Sensor changes not applied",
            "/sensors",
            "No Sensor Manager restart is pending, or a stronger runtime action takes priority.",
            false);
        return;
    }
    if (!runtimeManager_.applyPendingSensorChanges()) {
        sendResult(
            "Sensor changes not applied",
            "/sensors",
            "The Sensor runtime rebuild failed. The pending action remains active; see Diagnostics and logs.",
            false);
        return;
    }
    sendResult(
        "Sensor changes applied",
        "/sensors",
        "The complete Sensor runtime composition was rebuilt successfully.",
        true);
}
void WebService::handleRestart() { if(otaService_.busy()){sendResult("Restart unavailable","/firmware","A firmware upload is currently active.",false);return;} runtimeManager_.request(RuntimeAction::RestartDevice); sendResult("Restarting","/firmware","The device is restarting now.",true); performExplicitRestart(); }
void WebService::handleFactoryReset() { if(otaService_.busy()){sendResult("Factory reset unavailable","/firmware","A firmware upload is currently active.",false);return;} if(!configurationService_.resetToDefaults()){sendResult("Factory reset failed","/firmware","Stored configuration could not be cleared.",false);return;} runtimeManager_.request(RuntimeAction::RestartDevice); sendResult("Factory reset complete","/firmware","Configuration erased. Restarting into provisioning mode.",true);performExplicitRestart(); }
void WebService::handleNotFound() { server_.send(404,"text/plain","Not Found"); }
void WebService::performExplicitRestart() { server_.client().flush(); runtimeManager_.performPendingRestart(); }

} // namespace WeatherStation
