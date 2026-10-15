#include "ControllerTargetIdentity.h"
#include "WebService.h"
#include "WebLocalization.h"
#include "WebPresentationText.h"
#include "HtmlEscaping.h"
#include "LogWebView.h"
#include "PropertyResolver.h"
#include "PropertyPresentationReader.h"
#include "SystemPropertyReader.h"
#include "ValuePropertyReader.h"
#include "PropertyPreviewWebView.h"
#include "PageEditorScript.h"
#include "WebNavigation.h"
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
#include "DuoRelayDescriptor.h"
#include "InstanceUuid.h"

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

const char SharedStyle[] PROGMEM = R"CSS(
:root{--bg:#f5f5f5;--panel:#fff;--ink:#262626;--muted:#595959;--line:#d6d6d6;--brand:#9e6c00;--brand2:#805700;--good:#177245;--warn:#a55b00;--bad:#a32828}html,body,*,*::before,*::after{box-sizing:border-box}html,body{max-width:100%}body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.45 system-ui,-apple-system,sans-serif;overflow-x:hidden}.shell{min-height:100vh;min-width:0;display:grid;grid-template-columns:220px minmax(0,1fr)}.side{background:#262626;color:#fff;padding:22px 16px;min-width:0}.brand{font-weight:750;font-size:19px;margin:0 8px 4px;overflow-wrap:anywhere}.version{color:#d6d6d6;font-size:12px;margin:0 8px 20px}.nav a{display:block;color:#faf4e6;text-decoration:none;padding:9px 11px;border-radius:7px;margin:2px 0}.nav a:hover,.nav a.active{background:#9e6c00;color:#fff}.main{padding:28px;max-width:1100px;width:100%;min-width:0}.main.main-wide{max-width:none}.top{display:flex;justify-content:space-between;gap:16px;align-items:start;margin-bottom:22px;min-width:0}h1{font-size:25px;margin:0;overflow-wrap:anywhere}h2{font-size:17px;margin:0 0 14px}p{margin:8px 0;overflow-wrap:anywhere}.muted,.help,.secondary{color:var(--muted)}.help,.secondary{font-size:13px}.secondary{display:block;margin-top:4px}.grid,.summary-grid{display:grid;gap:16px;min-width:0;max-width:100%;margin-bottom:16px}.grid{grid-template-columns:repeat(auto-fit,minmax(min(240px,100%),1fr))}.summary-grid.primary{grid-template-columns:repeat(4,minmax(0,1fr))}.summary-grid.domain{grid-template-columns:repeat(3,minmax(0,1fr))}.grid>.card,.summary-grid>.card{margin-bottom:0}.card{background:var(--panel);border:1px solid var(--line);border-radius:10px;padding:18px;margin-bottom:16px;box-shadow:0 1px 2px #1122;min-width:0;max-width:100%;overflow:hidden}.kv{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1.4fr);gap:8px 14px;min-width:0;max-width:100%}.kv>span{min-width:0;max-width:100%;overflow-wrap:anywhere}.kv span:nth-child(odd){color:var(--muted)}.badge{display:inline-block;border-radius:99px;padding:3px 9px;font-size:12px;font-weight:700;background:#faf4e6;max-width:100%;white-space:nowrap;overflow-wrap:normal}.badge.good{color:var(--good);background:#e2f4ea}.badge.warn{color:var(--warn);background:#fff0d7}.badge.bad{color:var(--bad);background:#fbe3e3}.notice{border-left:4px solid var(--warn);background:#fff8e9;padding:11px 13px;border-radius:5px;margin-bottom:16px;max-width:100%;overflow-wrap:anywhere}.success{border-left-color:var(--good);background:#eaf7ef}.error{border-left-color:var(--bad);background:#fdecec}label{display:block;font-weight:650;margin:0 0 14px;min-width:0;max-width:100%;overflow-wrap:anywhere}input,select{display:block;width:100%;max-width:520px;min-width:0;margin-top:5px;padding:9px 10px;border:1px solid #d6d6d6;border-radius:6px;background:#fff;color:var(--ink);font:inherit}input[type=checkbox],input[type=radio]{display:inline;width:auto;margin:0 7px 0 0}.choice{font-weight:500;margin:7px 0}.actions{display:flex;gap:10px;flex-wrap:wrap;margin-top:18px;min-width:0}button,.button{border:0;border-radius:6px;padding:9px 15px;background:var(--brand);color:#fff;text-decoration:none;font:600 14px inherit;cursor:pointer;max-width:100%;white-space:nowrap}button:hover,.button:hover{background:var(--brand2)}button.danger{background:var(--bad)}table{width:100%;border-collapse:collapse;font-size:14px}th,td{text-align:left;padding:9px;border-bottom:1px solid var(--line);vertical-align:top;overflow-wrap:break-word}th{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.03em}.nowrap,.sensor-technical,.sensor-last-measurement,.sensor-actions{white-space:nowrap;overflow-wrap:normal}.sensor-technical,.sensor-last-measurement,.sensor-actions{width:1%}.table-scroll,.scroll{display:block;width:100%;max-width:100%;min-width:0;overflow-x:auto;overflow-y:hidden;-webkit-overflow-scrolling:touch}.table-actions{display:flex;gap:8px;align-items:center;justify-content:center;margin:0}.table-actions.vertical{flex-direction:column}.table-actions form{margin:0}.table-actions button,.table-action{min-width:max-content;white-space:nowrap}.measurement-table{min-width:620px}.measurement-table .measurement-name{width:auto}.measurement-table .measurement-value{min-width:130px;white-space:nowrap}.measurement-table .measurement-quality{min-width:90px;white-space:nowrap}.measurement-table .measurement-time{min-width:175px;white-space:nowrap}.actuator-table{min-width:1120px}.actuator-table th{white-space:nowrap;overflow-wrap:normal}.actuator-table td{vertical-align:middle}.actuator-table .actuator-slot{width:58px;white-space:nowrap}.actuator-table .actuator-name{min-width:145px}.actuator-table .actuator-configured{min-width:130px}.actuator-table .actuator-hardware{min-width:90px;white-space:nowrap}.actuator-table .actuator-runtime{min-width:175px}.actuator-table .actuator-initialization{min-width:120px;white-space:nowrap}.actuator-table .actuator-state{min-width:68px;white-space:nowrap}.actuator-table .actuator-controls{min-width:90px;text-align:center}.actuator-table .actuator-configure{min-width:112px;text-align:center}.actuator-table .actuator-controls button{min-width:58px}.controller-table{min-width:1320px}.controller-table th{white-space:nowrap;overflow-wrap:normal}.controller-table td{vertical-align:middle}.controller-table .controller-slot{width:58px;white-space:nowrap}.controller-table .controller-name{min-width:190px}.controller-table .controller-name .secondary{white-space:nowrap}.controller-table .controller-route{min-width:290px}.controller-table .controller-policy{min-width:210px}.controller-table .controller-runtime{min-width:150px}.controller-table .controller-diagnostics{min-width:245px}.controller-table .controller-controls{min-width:120px;text-align:center}.controller-table .controller-controls .table-actions{align-items:stretch}.controller-table .controller-controls button,.controller-table .controller-controls .button{width:100%}.diagnostic-stack{line-height:1.55}details{margin-top:12px;max-width:100%}summary{cursor:pointer;font-weight:650}h1,h2{color:var(--brand)}input[type=checkbox],input[type=radio]{accent-color:var(--brand)}:focus-visible{outline:2px solid var(--brand);outline-offset:3px}@media(max-width:900px){.summary-grid.primary{grid-template-columns:repeat(2,minmax(0,1fr))}}@media(max-width:760px){.shell{display:block}.side{padding:14px}.brand,.version{display:inline-block;margin:0 8px 10px 0}.nav{display:flex;overflow-x:auto;gap:3px}.nav a{white-space:nowrap}.main{padding:18px 13px}.top{display:block}.kv{grid-template-columns:minmax(0,1fr)}.kv span:nth-child(even){margin-bottom:7px}.card{padding:15px}.summary-grid.domain{grid-template-columns:1fr}}@media(max-width:480px){.summary-grid.primary{grid-template-columns:1fr}}
)CSS" R"CSS(
.log-summary{display:flex;align-items:flex-start;justify-content:space-between;gap:16px;margin-bottom:14px}.log-summary .help{margin-bottom:0}.log-table{min-width:620px;table-layout:fixed}.log-table .log-seq{width:84px;white-space:nowrap;overflow-wrap:normal}.log-table .log-time{width:190px;white-space:nowrap;overflow-wrap:normal}.log-table .log-level-cell{width:86px;white-space:nowrap;overflow-wrap:normal}.log-table .log-message{white-space:normal;overflow-wrap:anywhere}.log-level.debug{color:var(--muted)}.log-level.info{color:var(--brand)}@media(max-width:480px){.log-summary{display:block}.log-summary .button{display:inline-block;margin-top:10px}}
)CSS" R"CSS(
.log-refresh-controls{display:flex;align-items:center;justify-content:flex-end;gap:10px;flex-wrap:wrap}.log-refresh-controls .choice{margin:0}.log-refresh-controls .help{white-space:nowrap}@media(max-width:480px){.log-refresh-controls{justify-content:flex-start;margin-top:12px}}
)CSS" R"CSS(
.measurement-topic{font-size:12px;white-space:normal;overflow-wrap:anywhere;user-select:text}
)CSS";

struct TimezoneOption { const char* label; const char* value; };
const TimezoneOption TimezoneOptions[] = {
#include "TimezoneOptions.inc"
};

const char* webOtaStateName(OTAState state, Locale locale) {
    switch (state) {
        case OTAState::Idle: return webText(locale, TextId::Idle);
        case OTAState::Uploading: return webText(locale, TextId::Uploading);
        case OTAState::Verifying: return webText(locale, TextId::Verifying);
        case OTAState::FirmwareStaged: return webText(locale, TextId::FirmwareStaged);
        case OTAState::Failed: return webText(locale, TextId::Failed);
        default: return webText(locale, TextId::Unknown);
    }
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

String descriptorUuid(const uint8_t (&value)[16]) {
    static const char Hex[] = "0123456789abcdef";
    String result;
    result.reserve(36);
    for (size_t index = 0; index < sizeof(value); ++index) {
        if (index == 4 || index == 6 || index == 8 || index == 10) result += '-';
        result += Hex[value[index] >> 4];
        result += Hex[value[index] & 0x0F];
    }
    return result;
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

void sendTimezoneSelect(WebServer& server, const String& current, Locale locale) {
    // Keep the worldwide catalog out of contiguous page-sized heap allocations.
    String chunk;
    chunk.reserve(1200);
    chunk = "<select name='timezone'>";
    bool found = false;
    for (const auto& option : TimezoneOptions) {
        const bool selected = !found && current == option.value;
        found = found || selected;
        chunk += "<option value='" + escapeHtml(option.value) + "'" + (selected ? " selected" : "") + ">" + option.label + "</option>";
        if (chunk.length() >= 1024) {
            server.sendContent(chunk);
            chunk = "";
        }
    }
    if (!found && !current.isEmpty()) chunk += "<option value='" + escapeHtml(current) + (String("' selected>") + webText(locale, TextId::Custom) + " ") + escapeHtml(current) + "</option>";
    chunk += "</select>";
    server.sendContent(chunk);
}

String localeSelect(Locale current) {
    const Locale supported[] = {
        Locale::GermanGermany,
        Locale::EnglishUnitedKingdom,
        Locale::EnglishUnitedStates,
    };
    String html;
    html.reserve(300);
    html = "<select name='locale'>";
    for (const Locale locale : supported) {
        const char* key = localeKey(locale);
        html += "<option value='";
        html += key;
        html += "'";
        if (locale == current) html += " selected";
        html += ">";
        switch (locale) {
            case Locale::GermanGermany: html += "Deutsch (de-DE)"; break;
            case Locale::EnglishUnitedKingdom: html += "English (en-GB)"; break;
            case Locale::EnglishUnitedStates: html += "English (en-US)"; break;
        }
        html += "</option>";
    }
    html += "</select>";
    return html;
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
      logReader_(logReader),
      discoveryPublisher_(discoveryPublisher), runtimeManager_(runtimeManager), otaService_(otaService),
      i2cBusManager_(i2cBusManager),
      boardIdentityResolution_(boardIdentityResolution),
      boardProvisioningService_(boardProvisioningService),
      moduleDiscoveryService_(moduleDiscoveryService),
      moduleDescriptorProvisioningService_(moduleDescriptorProvisioningService) {}

void WebService::begin() {
    server_.on("/values", HTTP_GET, [this]() { handleValues(); });
    server_.on("/values/edit", HTTP_GET, [this]() { handleValueEdit(); });
    server_.on("/values/save", HTTP_POST, [this]() { handleValueSave(); });
    server_.on("/values/set", HTTP_POST, [this]() { handleValueSet(); });
    server_.on("/values/delete", HTTP_POST, [this]() { handleValueDelete(); });
    server_.on("/", HTTP_GET, [this]() { handleStatus(); });
    server_.on("/status", HTTP_GET, [this]() { handleStatus(); });
    server_.on("/sensors", HTTP_GET, [this]() { handleSensors(); });
    server_.on("/actuators", HTTP_GET, [this]() { handleActuators(); });
    server_.on("/controllers", HTTP_GET, [this]() { handleControllers(); });
    server_.on("/measurements", HTTP_GET, [this]() { handleMeasurements(); });
    server_.on("/display", HTTP_GET, [this]() { handleDisplay(); });
    server_.on("/display-editor.js", HTTP_GET, [this]() {
        // Stream static code directly from flash; never add it to the page's heap buffer.
        server_.sendHeader("Cache-Control", "no-store");
        server_.send_P(200, PSTR("application/javascript; charset=utf-8"), PageEditorScript, sizeof(PageEditorScript) - 1);
    });
    server_.on("/display/outputs/save", HTTP_POST, [this]() { handleDisplayOutputsSave(); });
    server_.on("/display/save", HTTP_POST, [this]() { handleDisplaySave(); });
    server_.on("/sensors/edit", HTTP_GET, [this]() { handleSensorEdit(); });
    server_.on("/actuators/edit", HTTP_GET, [this]() { handleActuatorEdit(); });
    server_.on("/controllers/edit", HTTP_GET, [this]() { handleControllerEdit(); });
    server_.on("/network", HTTP_GET, [this]() { handleNetwork(); });
    server_.on("/mqtt", HTTP_GET, [this]() { handleMqtt(); });
    server_.on("/time", HTTP_GET, [this]() { handleTime(); });
    server_.on("/units", HTTP_GET, [this]() { handleUnits(); });
    server_.on("/device", HTTP_GET, [this]() { handleDevice(); });
    server_.on("/hardware", HTTP_GET, [this]() { handleHardware(); });
    server_.on("/configuration", HTTP_GET, [this]() { handleConfiguration(); });
    server_.on("/configuration/download", HTTP_POST, [this]() { handleConfigurationDownload(); });
    server_.on("/configuration/preview", HTTP_POST,
        [this]() { handleConfigurationPreview(); }, [this]() { handleConfigurationUploadData(); });
    server_.on("/configuration/restore", HTTP_POST, [this]() { handleConfigurationRestore(); });
    server_.on("/configuration/cancel", HTTP_POST, [this]() {
        restoreCandidate_.reset(); restoreToken_ = String(); handleConfiguration();
    });
    server_.on("/diagnostics", HTTP_GET, [this]() { handleDiagnostics(); });
    server_.on("/diagnostics/i2c/scan", HTTP_POST, [this]() { handleI2CScan(); });
    server_.on("/logs", HTTP_GET, [this]() { handleLogs(); });
    server_.on("/logs/data", HTTP_GET, [this]() { handleLogData(); });
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
    server_.on("/pressure/save", HTTP_POST, [this]() { handlePressureReductionSave(); });
    server_.on("/location/save", HTTP_POST, [this]() { handleGeoPositionSave(); });
    server_.on("/device/save", HTTP_POST, [this]() { handleDeviceSave(); });
    server_.on("/hardware/board-identity/write", HTTP_POST,
        [this]() { handleBoardProvisioning(); });
    server_.on("/device/board-identity/write", HTTP_POST,
        [this]() { handleBoardProvisioning(); });
    server_.on("/hardware/module-descriptor/write", HTTP_POST,
        [this]() { handleModuleDescriptorProvisioning(); });
    server_.on("/device/module-descriptor/write", HTTP_POST,
        [this]() { handleModuleDescriptorProvisioning(); });
    server_.on("/sensors/save", HTTP_POST, [this]() { handleSensorSave(); });
    server_.on("/sensors/apply", HTTP_POST, [this]() { handleSensorApply(); });
    server_.on("/actuators/save", HTTP_POST, [this]() { handleActuatorSave(); });
    server_.on("/actuators/apply", HTTP_POST, [this]() { handleActuatorApply(); });
    server_.on("/actuators/on", HTTP_POST, [this]() { handleActuatorOn(); });
    server_.on("/actuators/off", HTTP_POST, [this]() { handleActuatorOff(); });
    server_.on("/actuators/level", HTTP_POST, [this]() { handleActuatorLevel(); });
    server_.on("/controllers/save", HTTP_POST, [this]() { handleControllerSave(); });
    server_.on("/controllers/apply", HTTP_POST, [this]() { handleControllerApply(); });
    server_.on("/controllers/start", HTTP_POST, [this]() { handleControllerStart(); });
    server_.on("/controllers/stop", HTTP_POST, [this]() { handleControllerStop(); });
    server_.on("/restart", HTTP_POST, [this]() { handleRestart(); });
    server_.on("/factory-reset", HTTP_POST, [this]() { handleFactoryReset(); });
    server_.onNotFound([this]() { handleNotFound(); });
    server_.begin();
    logger_.info("Web administration started");
}

void WebService::loop() {
    if (restoreCandidate_ && millis() - restorePreviewTime_ > 300000UL) {
        restoreCandidate_.reset(); restoreToken_ = String();
    }
    server_.handleClient();
}

bool WebService::administrationAvailable() const { return wifiService_.inSetupAccessPointMode() || wifiService_.connected(); }

void WebService::sendPage(const char* title, const char* activeRoute, const String& content,
    int status, bool wideContent) {
    if (!administrationAvailable()) {
        server_.send(503, "text/plain", uiText(TextId::AdministrationUnavailableWhileNetworkIsConnecting));
        return;
    }
    // Send the existing content without constructing a second full-page String.
    // Large display forms otherwise need several simultaneous contiguous copies.
    const String header = renderPageHeader(title, activeRoute, wideContent);
    static const char footer[] = "</main></div></body></html>";
    if (header.isEmpty() || content.isEmpty()) {
        logger_.errorf("Web page rendering failed: header=%u content=%u heap=%u largest=%u",
            static_cast<unsigned>(header.length()), static_cast<unsigned>(content.length()),
            static_cast<unsigned>(ESP.getFreeHeap()), static_cast<unsigned>(ESP.getMaxAllocHeap()));
        server_.send(503, "text/plain", uiText(TextId::PageRenderingFailedPleaseRetry));
        return;
    }
    server_.setContentLength(header.length() + content.length() + sizeof(footer) - 1);
    server_.send(status, "text/html; charset=utf-8", header);
    server_.sendContent(content);
    server_.sendContent(footer, sizeof(footer) - 1);
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
    content += (String("'>") + uiText(TextId::Back) + "</a>");
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

String WebService::otaStatusHtml() const {
    String html;
    html.reserve(520);
    html = (String("<div class='kv'><span>") + uiText(TextId::OTAState) + "</span><span>");
    html += webOtaStateName(otaService_.state(), configurationService_.getLocale());
    html += (String("</span><span>") + uiText(TextId::UploadStatus) + "</span><span>");
    if (otaService_.totalBytesKnown()) {
        html += localeFormatter_.formatNumber(otaService_.bytesReceived(), 0);
        html += " / ";
        html += localeFormatter_.formatNumber(otaService_.totalBytes(), 0);
        html += (String(" ") + uiText(TextId::Bytes2));
        html += localeFormatter_.formatNumber(otaService_.progressPercent(), 0);
        html += "%)";
    } else if (otaService_.bytesReceived() > 0) {
        html += localeFormatter_.formatNumber(otaService_.bytesReceived(), 0);
        html += (String(" ") + uiText(TextId::BytesReceived));
    } else {
        html += "—";
    }
    html += (String("</span><span>") + uiText(TextId::RestartRequired) + "</span><span>");
    html += uiText(otaService_.restartRequired() ? TextId::Yes : TextId::No);
    html += "</span>";
    if (otaService_.state() == OTAState::Failed && !otaService_.lastError().isEmpty()) {
        html += (String("<span>") + uiText(TextId::LastError) + "</span><span>");
        html += escapeHtml(otaService_.lastError());
        html += "</span>";
    }
    html += "</div>";
    return html;
}

const char* WebService::uiText(TextId id) const {
    return webText(configurationService_.getLocale(), id);
}

String WebService::renderPageHeader(const char* title, const char* active,
    bool wideContent) const {
    const Configuration& cfg = configurationService_.getConfiguration();
    String html;
    if (!html.reserve(2048)) return String();
    html = String("<!doctype html><html lang='") + webLanguageTag(cfg.locale.locale) + "'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>";
    html += escapeHtml(title);
    html += " · EnvNode</title><link rel='stylesheet' href='/style.css?build=";
    html += FirmwareBuildInfo::BuildTimestampUtc;
    html += "'>";
    html += buildWebUiScript(cfg.locale.locale);
    html += "</head><body><div class='shell'><aside class='side'><div class='brand'>";
    html += escapeHtml(cfg.device.name);
    html += "</div><div class='version'>EnvNode · v";
    html += FirmwareBuildInfo::SemanticVersion;
    html += "</div>";
    html += buildWebNavigationHtml(active, cfg.locale.locale);
    html += "</aside><main class='main";
    if (wideContent) html += " main-wide";
    html += "'><div class='top'><div><h1>";
    html += escapeHtml(title);
    html += "</h1>";
    if (wifiService_.inSetupAccessPointMode()) html += (String("<p class='muted'>") + uiText(TextId::SetupAccessPointMode) + "</p>");
    html += "</div></div>";
    html += pendingRuntimeActionHtml();
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

void WebService::handleNetwork() {
    const NetworkConfiguration& n = configurationService_.getConfiguration().network;
    const bool isStatic = n.addressMode == NetworkAddressMode::Static;
    String c;
    c.reserve(2200);
    c = (String("<section class='card'><h2>") + uiText(TextId::NetworkConfiguration) + "</h2><form method='post' action='/network/save' autocomplete='off'>");
    c += (String("<label>") + uiText(TextId::Hostname) + "<input name='hostname' required value='") + escapeHtml(n.hostname) + (String("'></label><label>") + uiText(TextId::WiFiSSID) + "<input name='wifiSSID' required value='") + escapeHtml(n.wifiSSID) + "'></label>";
    c += (String("<label class='choice'><input type='checkbox' name='changeWifiPassword' value='1'>") + uiText(TextId::ChangeWiFiPassword) + "</label><label>" + uiText(TextId::NewWiFiPassword) + "<input type='password' name='wifiPassword' autocomplete='new-password'></label>");
    c += (String("<label>") + uiText(TextId::AddressMode) + "<select name='addressMode' id='addressMode' onchange='toggleStatic()'><option value='dhcp'") + String(!isStatic?" selected":"") + ">DHCP</option><option value='static'" + String(isStatic?" selected":"") + (String(">") + uiText(TextId::StaticIPv4) + "</option></select></label>");
    const NetworkAddressSnapshot assigned = wifiService_.stationAddresses();
    c += "<div id='dhcpFields'>";
    c += String("<p class='help'>") + uiText(TextId::ActiveNetworkAddresses) + "</p>";
    const auto assignedField = [&](TextId label, const String& value) {
        c += String("<label>") + uiText(label) + "<input readonly value='";
        c += value.isEmpty() || value == "0.0.0.0" ? String("—") : escapeHtml(value);
        c += "'></label>";
    };
    assignedField(TextId::IPv4Address, assigned.ipAddress);
    assignedField(TextId::SubnetMask, assigned.subnetMask);
    assignedField(TextId::DefaultGateway, assigned.gateway);
    assignedField(TextId::PrimaryDNS, assigned.dns1);
    assignedField(TextId::SecondaryDNS, assigned.dns2);
    c += "</div>";
    c += (String("<div id='staticFields'><label>") + uiText(TextId::IPv4Address) + "<input name='ipv4Address' value='") + escapeHtml(n.ipv4Address) + (String("'></label><label>") + uiText(TextId::SubnetMask) + "<input name='subnetMask' value='") + escapeHtml(n.subnetMask) + (String("'></label><label>") + uiText(TextId::DefaultGateway) + "<input name='gateway' value='") + escapeHtml(n.gateway) + (String("'></label><label>") + uiText(TextId::PrimaryDNS) + "<input name='dns1' value='") + escapeHtml(n.dns1) + (String("'></label><label>") + uiText(TextId::SecondaryDNS) + " " + "<span class='help'>" + uiText(TextId::Optional) + "</span><input name='dns2' value='") + escapeHtml(n.dns2) + (String("'></label></div><div class='actions'><button>") + uiText(TextId::SaveNetworkSettings) + "</button></div></form></section>");
    c += "<script>function toggleStatic(){var s=document.getElementById('addressMode').value==='static',d=document.getElementById('staticFields');d.hidden=!s;document.getElementById('dhcpFields').hidden=s;d.querySelectorAll('input').forEach(function(i){i.disabled=!s})}toggleStatic()</script>";
    sendPage(uiText(TextId::Network), "/network", c);
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

void WebService::handleTime() {
    if (!administrationAvailable()) {
        server_.send(503, "text/plain", uiText(TextId::AdministrationUnavailableWhileNetworkIsConnecting));
        return;
    }
    const String header = renderPageHeader(uiText(TextId::LocaleTime), "/time", false);
    if (header.isEmpty()) {
        server_.send(503, "text/plain", uiText(TextId::PageRenderingFailedPleaseRetry));
        return;
    }
    const TimeConfiguration& t = configurationService_.getConfiguration().time;
    const Locale currentLocale = configurationService_.getLocale();
    String c;
    c.reserve(1300);
    c = (String("<section class='card'><h2>") + uiText(TextId::RuntimeStatus) + "</h2><div class='kv'><span>" + uiText(TextId::Synchronization) + "</span><span>") + badge(uiText(timeService_.synchronized()?TextId::Synchronized:TextId::Synchronizing),timeService_.synchronized()?"good":"warn") + (String("</span><span>") + uiText(TextId::LocalTime) + "</span><span>") + (timeService_.synchronized()?escapeHtml(currentLocalDateTime()):"—") + (String("</span></div></section><form method='post' action='/time/save'><section class='card'><h2>") + uiText(TextId::Locale) + "</h2><label>" + uiText(TextId::Locale) + "") + localeSelect(currentLocale) + (String("</label></section><section class='card'><h2>") + uiText(TextId::Time) + "</h2>");
    c += String("<label>") + uiText(TextId::Timezone);
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.send(200, "text/html; charset=utf-8", header);
    server_.sendContent(c);
    c = "";
    sendTimezoneSelect(server_, t.timezone, currentLocale);
    c = (String("</label><label>") + uiText(TextId::NTPServer1) + "<input name='ntpServer1' value='") + escapeHtml(t.ntpServer1) + (String("'></label><label>") + uiText(TextId::NTPServer2) + "<input name='ntpServer2' value='") + escapeHtml(t.ntpServer2) + (String("'></label><div class='actions'><button>") + uiText(TextId::SaveLocaleAndTime) + "</button></div></section></form>");
    server_.sendContent(c);
    server_.sendContent("</main></div></body></html>");
    server_.sendContent("");
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
        + uiText(TextId::RestoreHelp) + "</p><form method='post' action='/configuration/preview' enctype='multipart/form-data'>"
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
    } else if (upload.status == UPLOAD_FILE_WRITE && !configurationUploadFailed_) {
        if (upload.currentSize > MaxConfigurationBackupLength - configurationUpload_.length()
            || !configurationUpload_.concat(reinterpret_cast<const char*>(upload.buf), upload.currentSize)) {
            configurationUploadFailed_ = true; configurationUpload_ = String();
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        configurationUploadComplete_ = true;
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        configurationUploadFailed_ = false; configurationUploadComplete_ = false; configurationUpload_ = String();
        restoreCandidate_.reset(); restoreToken_ = String();
    }
}

void WebService::handleConfigurationPreview() {
    restoreCandidate_.reset(new (std::nothrow) Configuration);
    bool valid = !otaService_.busy() && !configurationUploadFailed_ && configurationUploadComplete_
        && restoreCandidate_ && decodeConfigurationBackup(configurationUpload_,
            configurationService_.getConfiguration(), *restoreCandidate_, restoreIncludesPasswords_)
        && configurationService_.validateRestore(*restoreCandidate_);
    configurationUpload_ = String(); configurationUploadFailed_ = false; configurationUploadComplete_ = false;
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
}

void WebService::handleConfigurationRestore() {
    if (otaService_.busy() || !restoreCandidate_ || server_.arg("confirmRestore") != "1"
        || server_.arg("token") != restoreToken_ || millis() - restorePreviewTime_ > 300000UL) {
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
    runtimeManager_.request(RuntimeAction::RestartDevice);
    String html = "<div class='notice success'><strong>" + String(uiText(TextId::RestoreSaved))
        + "</strong><p>" + uiText(TextId::RestoreRestart)
        + "</p></div><form method='post' action='/restart' onsubmit='return confirm(globalThis.EnvNodeUi.RestartTheDeviceNow)'><button>"
        + uiText(TextId::RestartNow) + "</button></form><a class='button' href='/configuration'>"
        + uiText(TextId::RestartLater) + "</a>";
    sendPage(uiText(TextId::RestoreSaved), "/configuration", html);
}

void WebService::handleDevice() {
    String c;
    c = (((String("<section class='card'><h2>") + uiText(TextId::DeviceIdentity) + "</h2><form method='post' action='/device/save'><label>")) + uiText(TextId::DeviceName) + "<input name='deviceName' required value='") + escapeHtml(configurationService_.getConfiguration().device.name) + (String("'></label><div class='actions'><button>") + uiText(TextId::SaveDeviceSettings) + "</button></div></form></section>");
    c += buildPressureReductionConfigurationHtml(configurationService_.getConfiguration(), configurationService_.getLocale());
    c += buildGeoPositionConfigurationHtml(configurationService_.getConfiguration().geoPosition,
        configurationService_.getLocale());
    c += "<section class='card'><h2>" + String(uiText(TextId::Hardware)) + "</h2><div class='kv'><span>"
        + uiText(TextId::BoardProfile) + "</span><span>" + escapeHtml(currentBoardProfile().displayName)
        + "</span><span>" + uiText(TextId::SerialNumber) + "</span><span>"
        + String(boardIdentityResolution_.identity.serialNumber) + "</span></div><div class='actions'><a class='button' href='/hardware'>"
        + uiText(TextId::ViewHardware) + "</a></div></section>";
    sendPage(uiText(TextId::Device), "/device", c);
}

void WebService::handleHardware() {
    String c;
    c.reserve(1500);
    const BoardProfile& board = currentBoardProfile();
    const BoardIdentity& identity = boardIdentityResolution_.identity;
    const bool recordFormatKnown =
        boardIdentityResolution_.recordStatus == BoardIdentityStatus::Valid
        || boardIdentityResolution_.recordStatus == BoardIdentityStatus::UnassignedSerial;
    const String serialNumber = identity.serialNumber == 0
        ? String(uiText(TextId::Unassigned))
        : localeFormatter_.formatNumber(identity.serialNumber, 0);
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
    c += (String("<section class='card'><h2>") + uiText(TextId::ProvisionModuleDescriptor) + "</h2>");
    c += (String("<div class='notice'><strong>ENHD descriptor 0.1</strong><p>") + uiText(TextId::ThisCreatesTheDeterministicDuoRelay03CBOR) + "</p></div>");
    c += "<form method='post' action='/hardware/module-descriptor/write' onsubmit='return confirm(globalThis.EnvNodeUi.WriteAndVerifyTheDuorelayDescriptorInThe)'>";
    c += (String("<label>") + uiText(TextId::ModuleSlot) + "<select name='descriptorModuleSlot' required><option value='A'>A</option><option value='B'>B</option></select></label>");
    c += (String("<label>") + uiText(TextId::DescriptorTemplate) + "<select name='descriptorTemplate' required><option value='duo-relay-0.3'>EnvNode DuoRelay 0.3</option></select></label>");
    c += (String("<p class='help'>") + uiText(TextId::APersistentUUIDVersion4IsGeneratedAutomatically) + "</p>");
    c += (String("<label>") + uiText(TextId::SerialNumberOptional) + "<input name='descriptorSerialNumber' maxlength='64'></label>");
    c += (String("<label>") + uiText(TextId::ProductionBatchOptional) + "<input name='descriptorProductionBatch' maxlength='64'></label>");
    c += (String("<label>") + uiText(TextId::ProductionDateOptional) + "<input type='date' name='descriptorProductionDate'></label>");
    c += (String("<label class='choice'><input type='checkbox' name='confirmModuleDescriptorProvisioning' value='1' required>") + uiText(TextId::IUnderstandThatThisWritesASelfDescribing) + "</label>");
    c += (String("<div class='actions'><button class='danger' type='submit'>") + uiText(TextId::WriteDuoRelayDescriptor) + "</button></div></form></section>");
    c += (String("<section class='card'><h2>") + uiText(TextId::ProvisionBoardIdentity) + "</h2>");
    c += (String("<div class='notice'><strong>") + uiText(TextId::AdvancedOperation) + "</strong><p>" + uiText(TextId::ThisWritesPermanentPhysicalBoardIdentityDataThe) + "</p></div>");
    c += "<form method='post' action='/hardware/board-identity/write' onsubmit='return confirm(globalThis.EnvNodeUi.WriteAndVerifyThisBoardIdentityTheActive)'>";
    c += (String("<label>") + uiText(TextId::BoardProfile) + "<select name='boardProfileId' required>");
    for (size_t index = 0; index < boardProfileCount(); ++index) {
        const BoardProfile* profile = boardProfileAt(index);
        if (profile == nullptr) continue;
        c += "<option value='" + String(encodeBoardProfileId(profile->id)) + "'";
        if (profile->id == identity.profileId) c += " selected";
        c += ">" + escapeHtml(profile->displayName) + "</option>";
    }
    c += "</select></label>";
    c += (String("<p class='help'>") + uiText(TextId::TheSelectedBoardNameIsStoredAsIts) + "</p>");
    c += (String("<label>") + uiText(TextId::BoardSerialNumber) + "<input type='number' name='serialNumber' min='0' max='4294967295' required value='") + String(identity.serialNumber) + "'></label>";
    c += (String("<p class='help'>") + uiText(TextId::SerialNumber0ExplicitlyMeansUnassigned) + "</p>");
    c += (String("<label class='choice'><input type='checkbox' name='confirmProvisioning' value='1' required>") + uiText(TextId::IUnderstandThatThisWritesManufacturingIdentityData) + "</label>");
    c += (String("<div class='actions'><button class='danger' type='submit'>") + uiText(TextId::WriteBoardIdentity) + "</button></div></form></section>");
    sendPage(uiText(TextId::Hardware), "/hardware", c);
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
    server_.sendHeader(uiText(TextId::Location), "/display"); server_.send(303);
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
    server_.sendHeader(uiText(TextId::Location), pageIndex == 0 ? "/display?page=1#text-preview" : "/display?page=2#text-preview");
    server_.send(303);
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

    SystemPropertyReader timeProperties(timeService_, localeFormatter_, FirmwareBuildInfo::SemanticVersion, FirmwareBuildInfo::BuildNumber, FirmwareBuildInfo::GitCommit, FirmwareBuildInfo::CompactIdentity, &configurationService_, &wifiService_);
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

void WebService::handleLogs() {
    sendPage(uiText(TextId::Logs), "/logs", buildRecentLogHtml(logReader_, configurationService_.getLocale()));
}

void WebService::handleLogData() {
    if (!administrationAvailable()) {
        server_.send(503, "application/json", "{\"error\":\"Administration unavailable\"}");
        return;
    }
    server_.send(200, "application/json; charset=utf-8", buildRecentLogJson(logReader_));
}

void WebService::handleFirmware() {
    String c;
    c.reserve(2000);
    const char* sourceStyle = FirmwareBuildInfo::SourceDirty ? "warn"
        : String(FirmwareBuildInfo::SourceState) == "clean" ? "good" : "warn";
    c = (String("<section class='card'><h2>") + uiText(TextId::RunningFirmware) + "</h2><div class='kv'><span>" + uiText(TextId::FirmwareVersion) + "</span><span>") + String(FirmwareBuildInfo::SemanticVersion) + (String("</span><span>") + uiText(TextId::BuildIdentity) + "</span><span>") + FirmwareBuildInfo::CompactIdentity + (String("</span><span>") + uiText(TextId::BuildNumber) + "</span><span>") + FirmwareBuildInfo::BuildNumber + (String("</span><span>") + uiText(TextId::GitCommit) + "</span><span>") + FirmwareBuildInfo::GitCommit + (String("</span><span>") + uiText(TextId::GitBranch) + "</span><span>") + escapeHtml(FirmwareBuildInfo::GitBranch) + (String("</span><span>") + uiText(TextId::BuildTimestamp) + "</span><span>") + FirmwareBuildInfo::BuildTimestampUtc + (String("</span><span>") + uiText(TextId::SourceState) + "</span><span>") + badge(FirmwareBuildInfo::SourceDirty ? uiText(TextId::Dirty) : String(FirmwareBuildInfo::SourceState) == "clean" ? uiText(TextId::Clean) : uiText(TextId::Unknown), sourceStyle) + (String("</span><span>") + uiText(TextId::StagedFirmware) + "</span><span>")+(otaService_.firmwareStaged()?uiText(TextId::ReadyForActivation):"—")+"</span></div>";
    if (otaService_.firmwareStaged()) c += (String("<p class='help'>") + uiText(TextId::StagedFirmwareVersionMetadataIsNotYetAvailable) + "</p>");
    c += (String("</section><section class='card'><h2>") + uiText(TextId::FirmwareUpdateStatus) + "</h2>") + otaStatusHtml() + "</section>";
    if (!otaService_.firmwareStaged() && !otaService_.busy()) {
        c += (String("<section class='card'><h2>") + uiText(TextId::UploadFirmware) + (String("</h2><p class='help'>") + uiText(TextId::BuildWith) + " " + "<code>pio run</code>" + uiText(TextId::ThenUpload) + " " + "<code>.pio/build/&lt;environment&gt;/firmware.bin</code>.</p><form method='post' action='/firmware/upload' enctype='multipart/form-data' onsubmit='document.getElementById(\"firmwareSize\").value=document.getElementById(\"firmwareFile\").files[0].size'><input type='hidden' id='firmwareSize' name='firmwareSize' value='0'><label>" + uiText(TextId::FirmwareBinary) + "<input id='firmwareFile' type='file' name='firmware' accept='.bin,application/octet-stream' required></label><div class='actions'><button>") + uiText(TextId::UploadFirmware) + "</button></div></form></section>");
    }
    c += (String("<section class='card'><h2>") + uiText(TextId::Restart) + (String("</h2><form method='post' action='/restart' onsubmit='return confirm(globalThis.EnvNodeUi.RestartTheDeviceNow)'><button>") + uiText(TextId::RestartNow) + "</button></form></section><section class='card'><h2>") + uiText(TextId::FactoryReset) + (String("</h2><p>") + uiText(TextId::ThisPermanentlyRemovesAllStoredConfigurationAndRestarts) + "</p><form method='post' action='/factory-reset' onsubmit='return confirm(globalThis.EnvNodeUi.EraseALLConfigurationAndRestart)'><button class='danger'>") + uiText(TextId::FactoryReset) + "</button></form></section>");
    sendPage(uiText(TextId::Firmware), "/firmware", c);
}

void WebService::handleFirmwareUploadData() {
    HTTPUpload& upload = server_.upload();
    if (upload.status == UPLOAD_FILE_START) {
        firmwareUploadRequestAccepted_ = false;
        firmwareUploadRequestError_ = String();
        if (otaService_.busy()) {
            firmwareUploadRequestError_ = uiText(TextId::AnotherFirmwareUploadIsAlreadyActive);
            return;
        }
        if (otaService_.firmwareStaged()) {
            firmwareUploadRequestError_ = uiText(TextId::FirmwareIsAlreadyStagedAndAwaitingRestart);
            return;
        }
        if (upload.name != "firmware" || upload.filename.isEmpty()
            || !upload.filename.endsWith(".bin")) {
            firmwareUploadRequestError_ = uiText(TextId::SelectAValidPlatformIOFirmwareBinFile);
            otaService_.rejectUpload(firmwareUploadRequestError_.c_str());
            return;
        }
        const size_t expectedSize = static_cast<size_t>(server_.arg("firmwareSize").toInt());
        firmwareUploadRequestAccepted_ = otaService_.beginUpload(expectedSize);
        if (!firmwareUploadRequestAccepted_) {
            firmwareUploadRequestError_ = otaService_.lastError().isEmpty()
                ? String(uiText(TextId::FirmwareUploadCouldNotBeStarted)) : otaService_.lastError();
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
        otaService_.abortUpload(uiText(TextId::FirmwareUploadWasAbortedBeforeCompletion));
        firmwareUploadRequestAccepted_ = false;
        firmwareUploadRequestError_ = otaService_.lastError();
    }
}

void WebService::handleFirmwareUpload() {
    if (firmwareUploadRequestAccepted_ && otaService_.firmwareStaged()) {
        String content;
        content.reserve(520);
        content = (String("<div class='notice success'><strong>") + uiText(TextId::FirmwareUploadedSuccessfully) + "</strong><p>" + uiText(TextId::FirmwareIsStaged) + "</p><p>" + uiText(TextId::DeviceRestartIsRequiredToActivateTheNew) + "</p></div><form method='post' action='/restart' onsubmit='return confirm(globalThis.EnvNodeUi.RestartTheDeviceNow)'><button>" + uiText(TextId::RestartNow) + "</button></form><a class='button' href='/firmware'>" + uiText(TextId::RestartLater) + "</a>");
        sendPage(uiText(TextId::FirmwareStaged), "/firmware", content);
        return;
    }
    const String reason = !firmwareUploadRequestError_.isEmpty()
        ? firmwareUploadRequestError_ : otaService_.lastError();
    String content;
    content.reserve(480 + reason.length());
    content = (String("<div class='notice error'><strong>") + uiText(TextId::FirmwareUpdateFailed2) + "</strong><p>");
    content += escapeHtml(reason.isEmpty() ? String(uiText(TextId::InvalidFirmwareUploadRequest)) : reason);
    content += (((String("</p><p>") + uiText(TextId::TheCurrentlyRunningFirmwareRemainsActive) + "</p></div><a class='button' href='/firmware'>")) + uiText(TextId::Back) + "</a>");
    sendPage(uiText(TextId::FirmwareUpdateFailed), "/firmware", content, 400);
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
    sendConfigurationResult(result,uiText(TextId::NetworkSettingsSaved),uiText(TextId::NetworkSaveFailed),"/network",uiText(TextId::InvalidNetworkSettingsStaticModeRequiresAValid));
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

void WebService::handleTimeSave() { const Configuration& current=configurationService_.getConfiguration(); Locale locale; bool ok=parseLocaleKey(server_.arg("locale").c_str(),locale); const bool localeChanged=ok&&locale!=current.locale.locale; const bool timeChanged=server_.arg("timezone")!=current.time.timezone||server_.arg("ntpServer1")!=current.time.ntpServer1||server_.arg("ntpServer2")!=current.time.ntpServer2; if(ok)ok=configurationService_.setLocale(locale)&&configurationService_.setTimezone(server_.arg("timezone"))&&configurationService_.setNtpServer1(server_.arg("ntpServer1"))&&configurationService_.setNtpServer2(server_.arg("ntpServer2")); sendConfigurationResult(configurationSaveResult(ok,ConfigurationArea::Locale,localeChanged,ConfigurationArea::Time,timeChanged),uiText(TextId::LocaleAndTimeSaved),uiText(TextId::LocaleAndTimeSaveFailed),"/time",uiText(TextId::InvalidLocaleOrTimeConfiguration)); }

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

void WebService::handleDeviceSave() { const bool ok=configurationService_.setDeviceName(server_.arg("deviceName")); sendConfigurationResult(configurationSaveResult(ok,ConfigurationArea::Device),uiText(TextId::DeviceSettingsSaved),uiText(TextId::DeviceSaveFailed),"/device",uiText(TextId::InvalidDeviceName)); }

void WebService::handleBoardProvisioning() {
    const String profileText = server_.arg("boardProfileId");
    char* profileEnd = nullptr;
    const unsigned long parsedProfile = strtoul(profileText.c_str(), &profileEnd, 10);
    BoardProfileId profileId = BoardProfileId::EnvNodeMainboard;
    const bool profileValid = !profileText.isEmpty()
        && profileEnd != profileText.c_str()
        && *profileEnd == '\0'
        && parsedProfile <= UINT16_MAX
        && decodeBoardProfileId(static_cast<uint16_t>(parsedProfile), profileId);
    const BoardProfile* requestedProfile = profileValid ? boardProfile(profileId) : nullptr;
    const String serialText = server_.arg("serialNumber");
    char* end = nullptr;
    const unsigned long long parsedSerial = strtoull(serialText.c_str(), &end, 10);
    const bool serialValid = !serialText.isEmpty()
        && end != serialText.c_str()
        && *end == '\0'
        && parsedSerial <= UINT32_MAX;
    if (requestedProfile == nullptr || !serialValid) {
        sendResult(uiText(TextId::BoardProvisioningFailed), "/hardware",
            uiText(TextId::TheSelectedBoardProfileOrBoardSerialNumber), false);
        return;
    }

    const BoardIdentity requested = {
        requestedProfile->id,
        requestedProfile->revision,
        static_cast<uint32_t>(parsedSerial),
    };
    const bool confirmed = server_.hasArg("confirmProvisioning")
        && server_.arg("confirmProvisioning") == "1";
    const BoardProvisioningResult result =
        boardProvisioningService_.provision(requested, confirmed);
    logger_.infof("Board provisioning result=%s profile=%u revision=%u.%u serial=%lu",
        boardProvisioningStatusName(result.status),
        static_cast<unsigned int>(requested.profileId),
        requested.revision.major,
        requested.revision.minor,
        static_cast<unsigned long>(requested.serialNumber));

    if (result.status == BoardProvisioningStatus::Success) {
        String content;
        content.reserve(600);
        content = (String("<div class='notice success'><strong>") + uiText(TextId::BoardIdentityWrittenAndVerified) + "</strong><p>" + uiText(TextId::TheActiveBoardProfileIsUnchangedForThisBoot) + "</p></div><form method='post' action='/restart' onsubmit='return confirm(globalThis.EnvNodeUi.RestartTheDeviceNow)'><button>" + uiText(TextId::RestartNow) + "</button></form><a class='button' href='/hardware'>" + uiText(TextId::RestartLater) + "</a>");
        sendPage(uiText(TextId::BoardIdentityProvisioned), "/hardware", content);
        return;
    }

    const char* message = uiText(TextId::BoardIdentityCouldNotBeWrittenAndVerified);
    switch (result.status) {
        case BoardProvisioningStatus::ConfirmationRequired:
            message = uiText(TextId::ExplicitConfirmationIsRequiredNoEEPROMDataWas);
            break;
        case BoardProvisioningStatus::InvalidIdentity:
            message = uiText(TextId::TheRequestedBoardIdentityIsNotSupportedNo);
            break;
        case BoardProvisioningStatus::WriteFailed:
            message = uiText(TextId::TheEEPROMDidNotAcceptTheWriteIt);
            break;
        case BoardProvisioningStatus::ReadbackFailed:
            message = uiText(TextId::TheEEPROMWriteCouldNotBeReadBack);
            break;
        case BoardProvisioningStatus::ReadbackInvalid:
            message = uiText(TextId::TheEEPROMReadbackFailedRecordValidationProvisioningWas);
            break;
        case BoardProvisioningStatus::ReadbackMismatch:
            message = uiText(TextId::TheEEPROMReadbackDidNotExactlyMatchThe);
            break;
        default:
            break;
    }
    sendResult(uiText(TextId::BoardProvisioningFailed), "/hardware", message, false);
}

void WebService::handleModuleDescriptorProvisioning() {
    const String slotText = server_.arg("descriptorModuleSlot");
    ModuleSlot slot = ModuleSlot::A;
    const bool slotValid = slotText == "A" || slotText == "B";
    if (slotText == "B") slot = ModuleSlot::B;
    const bool templateValid =
        server_.arg("descriptorTemplate") == "duo-relay-0.3";

    DuoRelayDescriptorManufacturingData manufacturing;
    const String serialNumber = server_.arg("descriptorSerialNumber");
    const String productionBatch = server_.arg("descriptorProductionBatch");
    const String productionDate = server_.arg("descriptorProductionDate");
    const bool manufacturingValid = serialNumber.length() <= 64
        && productionBatch.length() <= 64
        && productionDate.length() <= 10;
    if (!slotValid || !templateValid || !manufacturingValid) {
        sendResult(uiText(TextId::ModuleDescriptorProvisioningFailed), "/hardware",
            uiText(TextId::TheSlotDescriptorTemplateOrManufacturingDataIs),
            false);
        return;
    }
    esp_fill_random(manufacturing.instanceId, sizeof(manufacturing.instanceId));
    InstanceUuid::makeVersion4(manufacturing.instanceId);
    manufacturing.serialNumber = serialNumber.c_str();
    manufacturing.productionBatch = productionBatch.c_str();
    manufacturing.productionDate = productionDate.c_str();
    std::unique_ptr<uint8_t[]> payload(new (std::nothrow) uint8_t[DuoRelayDescriptor::MaximumEncodedSize]);
    if (!payload) {
        sendResult(uiText(TextId::ModuleDescriptorProvisioningFailed), "/hardware",
            uiText(TextId::PageRenderingFailedPleaseRetry), false);
        return;
    }
    size_t payloadSize = 0;
    const CompactCborStatus encodeStatus = DuoRelayDescriptor::encode(
        manufacturing, payload.get(), DuoRelayDescriptor::MaximumEncodedSize, payloadSize);
    if (encodeStatus != CompactCborStatus::Success) {
        logger_.errorf("DuoRelay descriptor encoding failed status=%s",
            compactCborStatusName(encodeStatus));
        sendResult(uiText(TextId::ModuleDescriptorProvisioningFailed), "/hardware",
            uiText(TextId::TheDuoRelayDescriptorCouldNotBeEncodedNo),
            false);
        return;
    }

    const bool confirmed = server_.hasArg("confirmModuleDescriptorProvisioning")
        && server_.arg("confirmModuleDescriptorProvisioning") == "1";
    const ModuleDescriptorProvisioningResult result =
        moduleDescriptorProvisioningService_.provision(
            slot, payload.get(), payloadSize, confirmed);
    logger_.infof(
        "Module descriptor provisioning result=%s slot=%s uuid=%s bytes=%u bank=%u generation=%lu decode=%s compatibility=%s",
        moduleDescriptorProvisioningStatusName(result.status), moduleSlotName(slot),
        descriptorUuid(manufacturing.instanceId).c_str(),
        static_cast<unsigned int>(payloadSize),
        static_cast<unsigned int>(result.bank),
        static_cast<unsigned long>(result.generation),
        hardwareDescriptorDecodeStatusName(result.decodeStatus),
        hardwareDescriptorCompatibilityStatusName(result.compatibilityStatus));

    if (result.status == ModuleDescriptorProvisioningStatus::Success) {
        const char* bank = result.bank == HardwareDescriptorBank::B ? "B" : "A";
        String message = (String(uiText(TextId::TheDuoRelayDescriptorWasEncodedValidatedWrittenTo)) + " ");
        message += bank;
        message += (String(uiText(TextId::VerifiedAndRediscoveredWithInstanceUuid)) + " ");
        message += descriptorUuid(manufacturing.instanceId);
        message += uiText(TextId::NoRestartRequired);
        sendResult(uiText(TextId::ModuleDescriptorProvisioned), "/hardware", message.c_str(), true);
        return;
    }

    const char* message = uiText(TextId::TheModuleDescriptorCouldNotBeWrittenAnd);
    switch (result.status) {
        case ModuleDescriptorProvisioningStatus::ConfirmationRequired:
            message = uiText(TextId::ExplicitConfirmationIsRequiredNoEEPROMDataWas);
            break;
        case ModuleDescriptorProvisioningStatus::InvalidSlot:
            message = uiText(TextId::TheSelectedModuleSlotIsInvalidNoEEPROM);
            break;
        case ModuleDescriptorProvisioningStatus::InvalidDescriptor:
            message = uiText(TextId::TheGeneratedDescriptorFailedSemanticValidationNoEEPROM);
            break;
        case ModuleDescriptorProvisioningStatus::IncompatibleDescriptor:
            message = uiText(TextId::TheDescriptorIsNotCompatibleWithTheActive);
            break;
        case ModuleDescriptorProvisioningStatus::StorageUnavailable:
            message = uiText(TextId::TheSelectedModuleEEPROMIsUnavailableCheckThat);
            break;
        case ModuleDescriptorProvisioningStatus::WriteFailed:
            message = uiText(TextId::TheSelectedModuleEEPROMDidNotAcceptThe);
            break;
        case ModuleDescriptorProvisioningStatus::VerificationFailed:
            message = uiText(TextId::TheDescriptorWriteFailedAtomicBankReadbackVerification);
            break;
        case ModuleDescriptorProvisioningStatus::RediscoveryFailed:
            message = uiText(TextId::TheDescriptorWasWrittenAndVerifiedButSemantic);
            break;
        default:
            break;
    }
    sendResult(uiText(TextId::ModuleDescriptorProvisioningFailed), "/hardware", message, false);
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
    server_.sendHeader(uiText(TextId::Location), "/actuators", true);
    server_.send(303, "text/plain", "See Other");
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
    server_.sendHeader(uiText(TextId::Location), "/actuators", true);
    server_.send(303, "text/plain", "See Other");
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
    server_.sendHeader(uiText(TextId::Location), "/actuators", true);
    server_.send(303, "text/plain", "See Other");
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
    server_.sendHeader(uiText(TextId::Location), "/controllers", true);
    server_.send(303, "text/plain", "See Other");
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
    server_.sendHeader(uiText(TextId::Location), "/controllers", true);
    server_.send(303, "text/plain", "See Other");
}
void WebService::handleRestart() { if(otaService_.busy()){sendResult(uiText(TextId::RestartUnavailable),"/firmware",uiText(TextId::AFirmwareUploadIsCurrentlyActive),false);return;} runtimeManager_.request(RuntimeAction::RestartDevice); sendResult(uiText(TextId::Restarting),"/firmware",uiText(TextId::TheDeviceIsRestartingNow),true); performExplicitRestart(); }
void WebService::handleFactoryReset() { if(otaService_.busy()){sendResult(uiText(TextId::FactoryResetUnavailable),"/firmware",uiText(TextId::AFirmwareUploadIsCurrentlyActive),false);return;} if(!configurationService_.resetToDefaults()){sendResult(uiText(TextId::FactoryResetFailed),"/firmware",uiText(TextId::StoredConfigurationCouldNotBeCleared),false);return;} runtimeManager_.request(RuntimeAction::RestartDevice); sendResult(uiText(TextId::FactoryResetComplete),"/firmware",uiText(TextId::ConfigurationErasedRestartingIntoProvisioningMode),true);performExplicitRestart(); }
void WebService::handleNotFound() { server_.send(404,"text/plain",uiText(TextId::NotFound)); }
void WebService::performExplicitRestart() { server_.client().flush(); runtimeManager_.performPendingRestart(); }

} // namespace EnvNode
