#include "WebService.h"
#include "HtmlEscaping.h"
#include "LogWebView.h"
#include "PropertyResolver.h"
#include "SystemPropertyReader.h"
#include "ValuePropertyReader.h"
#include "PropertyPreviewWebView.h"
#include "WebNavigation.h"
#include "ValueWebView.h"
#include "MqttTopic.h"
#include "BoardProfile.h"

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
    {"Europe/Berlin", "CET-1CEST,M3.5.0/2,M10.5.0/3"}, {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"UTC", "UTC0"}, {"America/New_York", "EST5EDT,M3.2.0/2,M11.1.0/2"},
    {"America/Chicago", "CST6CDT,M3.2.0/2,M11.1.0/2"}, {"America/Denver", "MST7MDT,M3.2.0/2,M11.1.0/2"},
    {"America/Los_Angeles", "PST8PDT,M3.2.0/2,M11.1.0/2"},
};

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
    const ModuleActuatorReference& reference) {
    for (size_t index = 0; index < ModuleDiscoveryService::SlotCount; ++index) {
        const auto* module = discovery.result(static_cast<ModuleSlot>(index));
        if (module == nullptr || !module->identified() || !module->descriptorHasInstanceId
            || memcmp(reference.moduleInstanceFingerprint, module->descriptorInstanceId,
                sizeof(reference.moduleInstanceFingerprint)) != 0) continue;
        return descriptorText(module->descriptorName.empty()
            ? module->descriptorTypeId : module->descriptorName)
            + " · Slot " + moduleSlotName(module->slot);
    }
    return "Module descriptor";
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

const char* sensorStateName(SensorState state) {
    switch (state) {
        case SensorState::Initializing: return "Initializing";
        case SensorState::Ready: return "Ready";
        case SensorState::Degraded: return "Degraded";
        case SensorState::Failed: return "Failed";
        default: return "Unknown";
    }
}

const char* onOffStateName(OnOffState state) {
    return state == OnOffState::On ? "On" : "Off";
}

const char* actuatorOperationFailure(ActuatorOperationResult result) {
    switch (result) {
        case ActuatorOperationResult::InvalidHardwareResource:
            return "The actuator hardware resource is invalid.";
        case ActuatorOperationResult::NotInitialized:
            return "The actuator is not initialized.";
        case ActuatorOperationResult::Completed:
            return "The actuator operation completed.";
        default:
            return "The actuator operation failed.";
    }
}

const char* blinkPhaseName(BlinkPhase phase) {
    switch (phase) {
        case BlinkPhase::Stopped: return "Stopped";
        case BlinkPhase::WaitingForTarget: return "Waiting for target";
        case BlinkPhase::On: return "On";
        case BlinkPhase::Off: return "Off";
        default: return "Unknown";
    }
}

const char* thresholdDecisionName(ThresholdDecision decision) {
    switch (decision) {
        case ThresholdDecision::On: return "On";
        case ThresholdDecision::Off: return "Off";
        case ThresholdDecision::Unknown:
        default: return "Unknown";
    }
}

const char* controllerOperationName(ControllerOperationResult result) {
    switch (result) {
        case ControllerOperationResult::Completed: return "Completed";
        case ControllerOperationResult::NoAction: return "No action required";
        case ControllerOperationResult::TargetUnavailable: return "Target unavailable";
        case ControllerOperationResult::ActuatorOperationFailed: return "Actuator operation failed";
        case ControllerOperationResult::InvalidConfiguration: return "Invalid configuration";
        case ControllerOperationResult::NotRunning: return "Not running";
        case ControllerOperationResult::ControllerNotFound: return "Controller not found";
        default: return "Unknown";
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

    const uint32_t ageMs = millis() - status.lastMeasurementMonotonicMs;
    result += "<br><span class='help'>";
    result += formatElapsedDuration(ageMs);
    result += " ago</span>";
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
    appendType(info.supportsHydrostaticPressure, MeasurementType::HydrostaticPressure);
    appendType(info.supportsWaterLevel, MeasurementType::WaterLevel);
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

String configuredHardwareAssignment(const HardwareResourceAssignment& hardware) {
    if (hardware.kind == HardwareResourceKind::GPIO) {
        return "GPIO" + String(hardware.gpio.number);
    }
    if (hardware.kind == HardwareResourceKind::I2C) {
        return String(i2cBusName(hardware.i2c.bus)) + " / 0x"
            + String(hardware.i2c.address, HEX);
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
        case MeasurementType::HydrostaticPressure: return info.supportsHydrostaticPressure;
        case MeasurementType::WaterLevel: return info.supportsWaterLevel;
        default: return false;
    }
}

const char* measurementQualityName(MeasurementQuality quality) {
    return measurementQualityDisplayName(quality);
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
    result += formatElapsedDuration(millis() - snapshot.acceptedMonotonicMs);
    result += " ago</span>";
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
    server_.on("/device/save", HTTP_POST, [this]() { handleDeviceSave(); });
    server_.on("/device/board-identity/write", HTTP_POST,
        [this]() { handleBoardProvisioning(); });
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
    server_.handleClient();
}

bool WebService::administrationAvailable() const { return wifiService_.inSetupAccessPointMode() || wifiService_.connected(); }

void WebService::sendPage(const char* title, const char* activeRoute, const String& content,
    int status, bool wideContent) {
    if (!administrationAvailable()) {
        server_.send(503, "text/plain", "Administration unavailable while network is connecting");
        return;
    }
    // Send the existing content without constructing a second full-page String.
    // Large display forms otherwise need several simultaneous contiguous copies.
    const String header = renderPageHeader(title, activeRoute, wideContent);
    static const char footer[] = "</main></div></body></html>";
    if (header.isEmpty() || content.isEmpty()) {
        logger_.error("Web page rendering failed: empty header or content");
        server_.send(503, "text/plain", "Page rendering failed. Please retry.");
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
        case RuntimeAction::RestartActuatorRuntime: return "Configuration saved. Actuator runtime apply required.";
        case RuntimeAction::RestartControllerRuntime: return "Configuration saved. Controller runtime apply required.";
        case RuntimeAction::RestartDevice: return "Configuration saved. Device restart required.";
        default: return "Configuration saved. Runtime action required.";
    }
}

String WebService::pendingRuntimeActionHtml() const {
    const RuntimeAction action = runtimeManager_.pendingAction();
    if (action == RuntimeAction::None) return String();
    String html;
    html.reserve(180);
    html = "<div class='notice'><strong>Runtime action required</strong><p>Pending runtime action: ";
    switch (action) {
        case RuntimeAction::RestartMqtt: html += "MQTT restart"; break;
        case RuntimeAction::RestartTime: html += "Time service restart"; break;
        case RuntimeAction::RestartWiFi: html += "WiFi restart"; break;
        case RuntimeAction::RestartSensorManager: html += "Sensor Manager restart"; break;
        case RuntimeAction::RestartActuatorRuntime: html += "Actuator runtime apply"; break;
        case RuntimeAction::RestartControllerRuntime: html += "Controller runtime apply"; break;
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

String WebService::renderPageHeader(const char* title, const char* active,
    bool wideContent) const {
    const Configuration& cfg = configurationService_.getConfiguration();
    String html;
    if (!html.reserve(2048)) return String();
    html = "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>";
    html += escapeHtml(title);
    html += " · EnvNode</title><link rel='stylesheet' href='/style.css?build=";
    html += FirmwareBuildInfo::BuildTimestampUtc;
    html += "'></head><body><div class='shell'><aside class='side'><div class='brand'>";
    html += escapeHtml(cfg.device.name);
    html += "</div><div class='version'>EnvNode · v";
    html += FirmwareBuildInfo::SemanticVersion;
    html += "</div>";
    html += buildWebNavigationHtml(active);
    html += "</aside><main class='main";
    if (wideContent) html += " main-wide";
    html += "'><div class='top'><div><h1>";
    html += escapeHtml(title);
    html += "</h1>";
    if (wifiService_.inSetupAccessPointMode()) html += "<p class='muted'>Setup access point mode</p>";
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
    c = "<div class='summary-grid primary'><section class='card'><h2>Device</h2><div class='kv'><span>Name</span><span>" + escapeHtml(cfg.device.name) + "</span><span>Firmware</span><span>" + FirmwareBuildInfo::SemanticVersion + "</span><span>Build</span><span>" + FirmwareBuildInfo::CompactIdentity + "</span><span>Uptime</span><span>" + localeFormatter_.formatNumber(millis()/1000UL, 0) + " seconds</span><span>Free heap</span><span>" + localeFormatter_.formatNumber(ESP.getFreeHeap(), 0) + " bytes</span><span>Flash</span><span>" + localeFormatter_.formatNumber(ESP.getFlashChipSize()/1024UL, 0) + " KB</span></div></section>";
    c += "<section class='card'><h2>Network</h2><div class='kv'><span>Status</span><span>" + (connected?badge("Connected","good"):setupAccessPoint?badge("Setup AP","warn"):badge("Disconnected","bad")) + "</span>";
    c += "<span>Connection mode</span><span>" + effectiveConnectionMode(setupAccessPoint) + "</span><span>Hostname</span><span>" + availableValue(hostname) + "</span><span>SSID</span><span>" + availableValue(ssid) + "</span>";
    c += "<span>IPv4 address</span><span>" + availableValue(ipv4Address) + "</span><span>Subnet mask</span><span>" + availableValue(subnetMask) + "</span><span>Default gateway</span><span>" + availableValue(gateway) + "</span>";
    c += "<span>DNS server 1</span><span>" + availableValue(dns1) + "</span>";
    if (!dns2.isEmpty() && dns2 != "0.0.0.0") c += "<span>DNS server 2</span><span>" + escapeHtml(dns2) + "</span>";
    c += "<span>MAC address</span><span>" + availableValue(macAddress) + "</span><span>RSSI</span><span>" + (connected?localeFormatter_.formatNumber(wifiService_.rssi(),0)+" dBm":"—") + "</span></div></section>";
    const bool configured = !cfg.mqtt.server.isEmpty();
    c += "<section class='card'><h2>MQTT</h2><div class='kv'><span>Configuration</span><span>" + badge(configured?"Configured":"Not configured",configured?"good":"warn") + "</span><span>Runtime</span><span>" + badge(mqttService_.connected()?"Connected":"Disconnected",mqttService_.connected()?"good":"bad") + "</span></div></section>";
    c += "<section class='card'><h2>Time</h2><div class='kv'><span>Status</span><span>" + badge(timeService_.synchronized()?"Synchronized":"Synchronizing",timeService_.synchronized()?"good":"warn") + "</span><span>Local time</span><span>" + (timeService_.synchronized()?escapeHtml(currentLocalDateTime()):"—") + "</span></div></section></div>";
    size_t runningControllerCount = 0;
    for (size_t index = 0; index < controllerRuntime_.runtimeCount(); ++index) {
        ControllerRuntimeInfo info;
        if (controllerRuntime_.runtimeInfo(index, info) && info.running) {
            ++runningControllerCount;
        }
    }
    c += "<div class='summary-grid domain'><section class='card domain-summary'><h2>Sensors</h2><div class='kv'><span>Active</span><span>"
        + localeFormatter_.formatNumber(sensorManager_.sensorCount(), 0)
        + "</span></div><p><a href='/sensors'>View Sensors</a></p></section>";
    c += "<section class='card domain-summary'><h2>Actuators</h2><div class='kv'><span>Active</span><span>"
        + localeFormatter_.formatNumber(actuatorRuntime_.runtimeCount(), 0)
        + "</span></div><p><a href='/actuators'>View Actuators</a></p></section>";
    c += "<section class='card domain-summary'><h2>Controllers</h2><div class='kv'><span>Active</span><span>"
        + localeFormatter_.formatNumber(controllerRuntime_.runtimeCount(), 0)
        + "</span><span>Running</span><span>"
        + localeFormatter_.formatNumber(runningControllerCount, 0)
        + "</span></div><p><a href='/controllers'>View Controllers</a></p></section></div>";
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
    c.reserve(1500);
    const BoardProfile& board = currentBoardProfile();
    const BoardIdentity& identity = boardIdentityResolution_.identity;
    const bool recordFormatKnown =
        boardIdentityResolution_.recordStatus == BoardIdentityStatus::Valid
        || boardIdentityResolution_.recordStatus == BoardIdentityStatus::UnassignedSerial;
    const String serialNumber = identity.serialNumber == 0
        ? String("Unassigned")
        : localeFormatter_.formatNumber(identity.serialNumber, 0);
    c = "<section class='card'><h2>Device identity</h2><form method='post' action='/device/save'><label>Device name<input name='deviceName' required value='" + escapeHtml(configurationService_.getConfiguration().device.name) + "'></label><div class='actions'><button>Save device settings</button></div></form></section>";
    c += "<section class='card'><h2>Board Identity</h2><div class='kv'>";
    c += "<span>Source</span><span>" + String(boardIdentitySourceName(boardIdentityResolution_.source)) + "</span>";
    c += "<span>Record status</span><span>" + String(boardIdentityStatusName(boardIdentityResolution_.recordStatus)) + "</span>";
    c += "<span>Board profile</span><span>" + escapeHtml(board.displayName) + "</span>";
    c += "<span>BoardProfileId</span><span>" + String(static_cast<unsigned int>(identity.profileId)) + "</span>";
    c += "<span>Hardware revision</span><span>" + String(identity.revision.major) + "." + String(identity.revision.minor) + "</span>";
    c += "<span>Serial number</span><span>" + serialNumber + "</span>";
    c += "<span>EEPROM format</span><span>" + String(recordFormatKnown ? "1" : "—") + "</span>";
    c += "<span>Normal runtime</span><span>" + String(boardIdentityResolution_.normalRuntimeAllowed ? "Allowed" : "Blocked") + "</span>";
    c += "</div>";
    if (boardIdentityResolution_.source == BoardIdentitySource::BuildFallback) {
        c += "<div class='notice'>No usable EEPROM Board Identity was selected. This boot uses the explicit development build fallback.</div>";
    }
    if (boardIdentityResolution_.recordStatus == BoardIdentityStatus::UnassignedSerial) {
        c += "<div class='notice'>The EEPROM Board Identity is valid, but its board serial number is unassigned.</div>";
    }
    c += "</section>";
    c += "<section class='card'><h2>Module Identity</h2><div class='scroll'><table><thead><tr><th>Slot</th><th>EEPROM</th><th>Source</th><th>Status</th><th>Module</th><th>Revision</th><th>Instance UUID</th><th>Serial number</th><th>Production batch</th><th>Production date</th></tr></thead><tbody>";
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
                ? String("Unassigned") : escapeHtml(descriptorText(module->descriptorSerialNumber)))
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
    c += "<section class='card'><h2>Discovered Module Devices</h2><div class='scroll'><table><thead><tr><th>Slot</th><th>Device</th><th>Kind</th><th>Driver</th><th>Capability</th><th>Binding</th><th>Board resource</th><th>Parameters</th><th>Status</th></tr></thead><tbody>";
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
                } else resourceText += "Resolved";
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
            c += "</td><td>" + String(moduleDeviceKindName(entry.kind));
            c += "</td><td>" + String(moduleDeviceDriverName(entry.driver));
            c += " v" + String(entry.driver.apiVersion);
            c += "</td><td>" + capability + "</td><td>" + bindingText;
            c += "</td><td>" + resourceText + "</td><td>" + parameters;
            c += "</td><td>" + String(moduleDeviceInventoryStatusName(entry.status));
            c += "</td></tr>";
        }
    }
    if (!moduleDeviceInventory) {
        c += "<tr><td colspan='9'>Insufficient memory to render module device inventory.</td></tr>";
    } else if (!hasModuleDevices) {
        c += "<tr><td colspan='9'>No compatible descriptor-defined devices discovered.</td></tr>";
    }
    moduleDeviceInventory.reset();
    c += "</tbody></table></div><p class='help'>This is a read-only inventory derived from module descriptors and logical slot resources. It does not modify the saved sensor or actuator configuration.</p></section>";
    c += "<section class='card'><h2>Provision Module Descriptor</h2>";
    c += "<div class='notice'><strong>ENHD descriptor 0.1</strong><p>This creates the deterministic DuoRelay 0.3 CBOR descriptor, validates it against the active board and firmware, writes it atomically, reads it back, and reruns discovery.</p></div>";
    c += "<form method='post' action='/device/module-descriptor/write' onsubmit='return confirm(\"Write and verify the DuoRelay descriptor in the selected slot?\")'>";
    c += "<label>Module slot<select name='descriptorModuleSlot' required><option value='A'>A</option><option value='B'>B</option></select></label>";
    c += "<label>Descriptor template<select name='descriptorTemplate' required><option value='duo-relay-0.3'>EnvNode DuoRelay 0.3</option></select></label>";
    c += "<p class='help'>A persistent UUID version 4 is generated automatically by the firmware for this physical module.</p>";
    c += "<label>Serial number (optional)<input name='descriptorSerialNumber' maxlength='64'></label>";
    c += "<label>Production batch (optional)<input name='descriptorProductionBatch' maxlength='64'></label>";
    c += "<label>Production date (optional)<input type='date' name='descriptorProductionDate'></label>";
    c += "<label class='choice'><input type='checkbox' name='confirmModuleDescriptorProvisioning' value='1' required>I understand that this writes a self-describing hardware descriptor to the selected module EEPROM.</label>";
    c += "<div class='actions'><button class='danger' type='submit'>Write DuoRelay Descriptor</button></div></form></section>";
    c += "<section class='card'><h2>Provision Board Identity</h2>";
    c += "<div class='notice'><strong>Advanced operation</strong><p>This writes permanent physical-board identity data. The active BoardProfile will not change until the device is restarted.</p></div>";
    c += "<form method='post' action='/device/board-identity/write' onsubmit='return confirm(\"Write and verify this Board Identity? The active profile changes only after restart.\")'>";
    c += "<label>Board profile<select name='boardProfileId' required>";
    for (size_t index = 0; index < boardProfileCount(); ++index) {
        const BoardProfile* profile = boardProfileAt(index);
        if (profile == nullptr) continue;
        c += "<option value='" + String(encodeBoardProfileId(profile->id)) + "'";
        if (profile->id == identity.profileId) c += " selected";
        c += ">" + escapeHtml(profile->displayName) + "</option>";
    }
    c += "</select></label>";
    c += "<p class='help'>The selected board name is stored as its stable numeric BoardProfileId. The supported hardware revision is taken from the firmware profile.</p>";
    c += "<label>Board serial number<input type='number' name='serialNumber' min='0' max='4294967295' required value='" + String(identity.serialNumber) + "'></label>";
    c += "<p class='help'>Serial number 0 explicitly means unassigned.</p>";
    c += "<label class='choice'><input type='checkbox' name='confirmProvisioning' value='1' required>I understand that this writes manufacturing identity data.</label>";
    c += "<div class='actions'><button class='danger' type='submit'>Write Board Identity</button></div></form></section>";
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
        c += "</td><td class='sensor-technical'>" + configuredHardwareAssignment(slot.hardware) + "</td><td class='sensor-technical'>";
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

void WebService::handleActuators() {
    String c;
    c.reserve(1300 + MaxActuatorSlotCount * 520);
    if (runtimeManager_.pendingAction() == RuntimeAction::RestartActuatorRuntime) {
        c = "<div class='notice'><strong>Actuator apply required</strong><p>Saved actuator configuration differs from the active runtime composition.</p><form method='post' action='/actuators/apply'><button>Apply Actuator Changes</button></form></div>";
    }
    c += "<section class='card'><h2>Actuator Slots</h2><p class='help'>Saved configuration is activated with Apply Actuator Changes. Runtime controls operate the currently active actuator.</p><div class='table-scroll actuator-table-wrap'><table class='actuator-table'><thead><tr><th class='actuator-slot'>Slot</th><th class='actuator-name'>Name</th><th class='actuator-configured'>Configured</th><th class='actuator-hardware'>Hardware</th><th class='actuator-runtime'>Runtime</th><th class='actuator-initialization'>Initialization</th><th class='actuator-state'>State</th><th class='actuator-controls'>Controls</th><th class='actuator-configure'></th></tr></thead><tbody>";
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
        c += (descriptorRuntime && !savedModule ? true : slot.enabled) ? badge("Enabled", "good") : badge("Disabled", "warn");
        c += "<br>" + escapeHtml(module ? "Module On/Off" : metadata == nullptr ? "Invalid" : metadata->displayType);
        c += "</td><td class='actuator-hardware'>"
            + (module ? escapeHtml(moduleActuatorLabel(moduleDiscoveryService_, moduleReference)) : configuredHardwareAssignment(slot.hardware)) + "</td><td class='actuator-runtime'>";
        if (hasRuntime) {
            c += escapeHtml(runtime.name) + " / "
                + configuredHardwareAssignment(runtime.hardware);
            if (descriptorRuntime) {
                c += "<br>" + badge("Module descriptor", "good")
                    + "<span class='secondary'>Slot "
                    + escapeHtml(moduleSlotName(runtime.moduleSlot)) + " / "
                    + escapeHtml(runtime.descriptorDeviceId) + "</span>";
            }
        } else {
            c += savedModule ? "Module disabled or unavailable" : "No runtime Actuator";
        }
        if (!runtimeMatches) c += "<br>" + badge("Actuator apply required", "warn");
        c += "</td><td class='actuator-initialization'>";
        if (!hasRuntime) c += "—";
        else if (runtime.available) c += badge("Initialized", "good");
        else if (runtime.initializationAttempted) c += badge("Failed", "bad");
        else c += badge("Construction failed", "bad");
        c += "</td><td class='actuator-state'>";
        c += onOff == nullptr ? "—" : onOffStateName(onOff->state());
        if (level != nullptr) {
            c += "<span class='secondary'>Level "
                + String(level->level().percent()) + " %</span>";
        }
        c += "</td><td class='actuator-controls'>";
        if (onOff != nullptr) {
            c += "<div class='table-actions vertical'><form method='post' action='/actuators/on'><input type='hidden' name='slot' value='"
                + String(slot.slotId) + "'><button type='submit'>On</button></form>"
                + "<form method='post' action='/actuators/off'><input type='hidden' name='slot' value='"
                + String(slot.slotId) + "'><button type='submit'>Off</button></form></div>";
            if (level != nullptr) {
                c += "<form method='post' action='/actuators/level'><input type='hidden' name='slot' value='"
                    + String(slot.slotId) + "'><label class='secondary'>Level %<input type='number' name='level' min='0' max='100' required value='"
                    + String(level->level().percent())
                    + "'></label><button type='submit'>Set Level</button></form>";
            }
        } else {
            c += "—";
        }
        c += "</td><td class='actuator-configure'><a class='button table-action' href='/actuators/edit?slot="
            + String(slot.slotId) + "'>Configure</a></td></tr>";
    }
    c += "</tbody></table></div></section>";
    sendPage("Actuators", "/actuators", c, 200, true);
}

void WebService::handleControllers() {
    String c;
    c.reserve(1500 + MaxControllerSlotCount * 900);
    if (runtimeManager_.pendingAction() == RuntimeAction::RestartControllerRuntime) {
        c = "<div class='notice'><strong>Controller apply required</strong><p>Saved Controller configuration differs from the active runtime composition.</p><form method='post' action='/controllers/apply'><button>Apply Controller Changes</button></form></div>";
    }
    c += "<section class='card'><h2>Controller Slots</h2><p class='help'>Saved configuration is activated with Apply Controller Changes. Start and Stop affect only the active runtime and do not change saved configuration.</p><div class='table-scroll controller-table-wrap'><table class='controller-table'><thead><tr><th class='controller-slot'>Slot</th><th class='controller-name'>Controller</th><th class='controller-route'>Source / Target</th><th class='controller-policy'>Policy</th><th class='controller-runtime'>Runtime</th><th class='controller-diagnostics'>Diagnostics</th><th class='controller-controls'>Controls</th></tr></thead><tbody>";
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
        c += slot.enabled ? badge("Enabled", "good") : badge("Disabled", "warn");
        c += "<span class='secondary'>" + escapeHtml(
            metadata == nullptr ? "Invalid" : metadata->displayType) + "</span>";
        c += "</td><td class='controller-route'>";
        if (slot.implementation == ControllerImplementation::Blink) {
            if (validModuleActuatorReference(slot.moduleTarget)) {
                c += "Target: Module actuator · "
                    + (moduleTargetName.isEmpty()
                        ? String("currently unavailable") : escapeHtml(moduleTargetName));
            } else {
                c += "Target: Actuator " + String(blink.targetActuatorId);
            }
            if (!validModuleActuatorReference(slot.moduleTarget)
                && isValidActuatorId(blink.targetActuatorId)
                && blink.targetActuatorId <= MaxActuatorSlotCount) {
                c += " · " + escapeHtml(configuration.actuatorSlots[
                    blink.targetActuatorId - 1].name);
            }
        } else if (slot.implementation == ControllerImplementation::Threshold) {
            c += String(threshold.direction == ThresholdDirection::OnAbove
                ? "On above / Off below" : "On below / Off above");
            c += "<br>";
            c += "Source: Sensor " + String(threshold.source.sensorId);
            if (isValidSensorId(threshold.source.sensorId)
                && threshold.source.sensorId <= MaxSensorSlotCount) {
                c += " · " + escapeHtml(configuration.sensorSlots[
                    threshold.source.sensorId - 1].name);
            }
            c += "<br>Measurement: ";
            c += escapeHtml(measurementTypeMetadata(
                threshold.source.measurementType).displayName);
            if (threshold.decisionOnly) {
                c += "<br>Decision only · no actuator";
            } else if (validModuleActuatorReference(slot.moduleTarget)) {
                c += "<br>Target: Module actuator · "
                    + (moduleTargetName.isEmpty()
                        ? String("currently unavailable") : escapeHtml(moduleTargetName));
            } else {
                c += "<br>Target: Actuator " + String(threshold.targetActuatorId);
            }
            if (!threshold.decisionOnly && !validModuleActuatorReference(slot.moduleTarget)
                && isValidActuatorId(threshold.targetActuatorId)
                && threshold.targetActuatorId <= MaxActuatorSlotCount) {
                c += " · " + escapeHtml(configuration.actuatorSlots[
                    threshold.targetActuatorId - 1].name);
            }
        } else if (slot.implementation == ControllerImplementation::Selector) {
            const auto& selector = slot.implementationConfiguration.selector;
            c += "Mode: Value " + String(selector.modeValueId) + "<br>Auto: Controller "
                + String(selector.automaticControllerId) + "<br>Target: "
                + (validModuleActuatorReference(slot.moduleTarget) ? String("Module actuator") : "Actuator " + String(selector.targetActuatorId));
        } else {
            c += "—";
        }
        c += "</td><td class='controller-policy'>";
        if (slot.implementation == ControllerImplementation::Blink) {
            c += String(blink.onDurationMs) + " ms On<br>" + String(blink.offDurationMs) + " ms Off";
        } else if (slot.implementation == ControllerImplementation::Threshold) {
            const char* unit = UnitConverter::symbol(
                measurementTypeMetadata(threshold.source.measurementType).canonicalUnit);
            if (unit == nullptr) unit = "";
            c += "On " + String(thresholdOnComparisonSymbol(threshold.direction))
                + " " + String(threshold.onThreshold, 4) + " " + unit;
            c += "<br>Off " + String(thresholdOffComparisonSymbol(threshold.direction))
                + " " + String(threshold.offThreshold, 4) + " " + unit;
            c += "<br>Max age " + String(threshold.maxMeasurementAgeMs) + " ms";
        } else if (slot.implementation == ControllerImplementation::Selector) {
            const auto& selector = slot.implementationConfiguration.selector;
            c += "Auto: " + escapeHtml(selector.automaticCode) + "<br>On: " + escapeHtml(selector.onCode)
                + "<br>Off: " + escapeHtml(selector.offCode) + "<br>Unknown: hold";
        } else {
            c += "—";
        }
        c += "</td><td class='controller-runtime'>";
        if (!hasRuntime) {
            c += "No runtime Controller";
        } else {
            c += runtime.running ? badge("Running", "good") : badge("Stopped", "warn");
            c += "<span class='secondary'>Active runtime</span>";
        }
        if (!runtimeMatches) c += "<br>" + badge("Controller apply required", "warn");
        c += "</td><td class='controller-diagnostics'><div class='diagnostic-stack'>";
        if (!hasRuntime) {
            c += "—";
        } else if (runtime.implementation == ControllerImplementation::Blink) {
            c += runtime.targetAvailable ? "Target available" : "Target unavailable";
            c += "<br>" + String(blinkPhaseName(runtime.blinkPhase));
            c += "<br>" + String(controllerOperationName(runtime.lastOperationResult));
        } else if (runtime.implementation == ControllerImplementation::Threshold) {
            if (!runtime.hasLatestSnapshot) {
                c += "No current Measurement";
            } else if (runtime.latestSnapshotStale) {
                c += "Stale Measurement ("
                    + formatElapsedDuration(runtime.latestSnapshotAgeMs) + " ago)";
            } else if (!runtime.latestMeasurementValid) {
                c += "Invalid Measurement";
            } else if (!runtime.sourceAvailable) {
                c += "Unusable Measurement";
            } else {
                c += "Usable Measurement";
                if (runtime.latestNumericValueAvailable) {
                    c += ": " + String(runtime.latestNumericValue, 4);
                }
            }
            c += "<br>Decision: ";
            c += runtime.thresholdDecision == ThresholdDecision::Unknown
                ? badge("Unknown", "")
                : runtime.thresholdDecision == ThresholdDecision::On
                    ? badge("On", "good") : badge("Off", "warn");
            c += "<br>";
            c += runtime.decisionOnly ? "Decision only" : runtime.targetAvailable ? "Target available" : "Target unavailable";
            if (runtime.outputApplicationPending) {
                c += "<br>" + badge("Output application pending", "warn");
            }
            c += "<br>" + String(controllerOperationName(runtime.lastOperationResult));
        } else if (runtime.implementation == ControllerImplementation::Selector) {
            c += runtime.thresholdDecision == ThresholdDecision::Unknown ? "Holding output (no decision)"
                : runtime.thresholdDecision == ThresholdDecision::On ? "Selected: On" : "Selected: Off";
            c += runtime.targetAvailable ? "<br>Target available" : "<br>Target unavailable";
            if (runtime.outputApplicationPending) c += "<br>Output application pending";
            c += "<br>" + String(controllerOperationName(runtime.lastOperationResult));
        } else {
            c += "Unsupported runtime implementation";
        }
        c += "</div></td><td class='controller-controls'>";
        c += "<div class='table-actions vertical'>";
        if (hasRuntime) {
            const char* action = runtime.running ? "stop" : "start";
            const char* label = runtime.running ? "Stop" : "Start";
            c += "<form method='post' action='/controllers/" + String(action)
                + "'><input type='hidden' name='slot' value='" + String(slot.slotId)
                + "'><button type='submit'>" + label + "</button></form>";
        }
        c += "<a class='button table-action' href='/controllers/edit?slot="
            + String(slot.slotId) + "'>Configure</a></div></td></tr>";
    }
    c += "</tbody></table></div></section>";
    sendPage("Controllers", "/controllers", c, 200, true);
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
        const char* fields[] = {"displayEnabled", "displayType", "displayBus", "displayAddress", "displayPage"};
        for (const char* field : fields) if (!server_.hasArg(String(field) + suffix)) {
            sendResult("Display save failed", "/display", "Incomplete display settings; previous settings retained.", false); return;
        }
    }
    std::unique_ptr<DisplayConfiguration> storage(new DisplayConfiguration(configurationService_.getConfiguration().display));
    auto& candidate = *storage;
    if (!readDisplayOutputs(candidate)) {
        sendResult("Display save failed", "/display", "Invalid display type, bus, address or page.", false); return;
    }
    if (candidate.hardware.enabled && candidate.secondHardware.enabled
        && candidate.hardware.bus == candidate.secondHardware.bus
        && candidate.hardware.address == candidate.secondHardware.address) {
        sendResult("Display save failed", "/display", "Both enabled displays use the same I2C bus and address. Choose a different bus or address; previous settings retained.", false);
        return;
    }
    if (!configurationService_.setDisplayConfiguration(candidate)) {
        sendResult("Display save failed", "/display", "Could not store display settings (storage failure or I2C address conflict); previous settings retained.", false); return;
    }
    server_.sendHeader("Location", "/display"); server_.send(303);
}

void WebService::handleDisplaySave() {
    if (server_.hasArg("page") && server_.arg("page") != "1" && server_.arg("page") != "2") {
        sendResult("Display save failed", "/display", "Invalid page.", false); return;
    }
    const size_t pageIndex = server_.arg("page") == "2" ? 1 : 0;
    std::unique_ptr<DisplayConfiguration> storage(new DisplayConfiguration(configurationService_.getConfiguration().display));
    auto& candidate = *storage;
    auto& editedPage = candidate.page(pageIndex);
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        const String number(static_cast<unsigned int>(line));
        const String formatField = "f" + number;
        if (!server_.hasArg(formatField)) {
            sendResult("Display save failed", "/display", "Incomplete display page; previous settings retained.", false);
            return;
        }
        editedPage.formats[line] = server_.arg(formatField);
        for (size_t source = 0; source < MaxPropertySourcesPerLine; ++source) {
            const String field = "s" + number + "_" + String(static_cast<unsigned int>(source));
            if (!server_.hasArg(field)) {
                sendResult("Display save failed", "/display", "Incomplete display page; previous settings retained.", false);
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
        sendResult("Display save failed", "/display", "Invalid format, sources or Boolean text. Match placeholders and sources without gaps; labels require a source and allow at most 16 UTF-8 bytes without control characters. Previous settings retained.", false);
        return;
    }
    if (!configurationService_.setDisplayConfiguration(candidate)) {
        sendResult("Display save failed", "/display", "Could not store display settings (storage failure or I2C address conflict); previous settings retained.", false);
        return;
    }
    server_.sendHeader("Location", pageIndex == 0 ? "/display?page=1#text-preview" : "/display?page=2#text-preview");
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
        content += "</p><div class='table-scroll measurement-table-wrap'><table class='measurement-table'><thead><tr><th class='measurement-name'>Measurement</th><th class='measurement-value'>Current Value</th><th class='measurement-quality'>Quality</th><th class='measurement-time'>Last Accepted</th></tr></thead><tbody>";
        for (uint8_t typeValue = 1; typeValue <= SupportedMeasurementTypeCount; ++typeValue) {
            const MeasurementType type = static_cast<MeasurementType>(typeValue);
            if (!runtimeSupportsMeasurement(runtime, type)) continue;
            MeasurementSnapshot snapshot;
            const bool hasSnapshot = measurementSnapshotCache_.snapshot(runtime.id, type, snapshot);
            content += "<tr><td class='measurement-name'>";
            content += measurementTypeMetadata(type).displayName;
            const String mqttTopic = mqttMeasurementTopic(
                configuration.device.name, runtime.id, type);
            content += "<span class='secondary'>MQTT: <code class='measurement-topic'>";
            content += mqttTopic.isEmpty() ? "—" : escapeHtml(mqttTopic);
            content += "</code></span>";
            content += "</td><td class='measurement-value'>";
            content += hasSnapshot
                ? presentedMeasurementValue(snapshot.measurement, configuration, localeFormatter_)
                : String("—");
            content += "</td><td class='measurement-quality'>";
            content += hasSnapshot ? measurementQualityName(snapshot.measurement.quality) : "—";
            content += "</td><td class='measurement-time'>";
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

void WebService::handleDisplay() {
    server_.sendHeader("Cache-Control", "no-store");
    String content;
    const Configuration& configuration = configurationService_.getConfiguration();
    if (!server_.hasArg("page") && !server_.hasArg("f0") && !server_.hasArg("source")
        && !server_.hasArg("format") && !server_.hasArg("preview") && !server_.hasArg("loadDisplay")) {
        sendPage("Display", "/display", buildDisplayOutputsHtml(configuration.display));
        return;
    }

    SystemPropertyReader timeProperties(timeService_, localeFormatter_, FirmwareBuildInfo::SemanticVersion, FirmwareBuildInfo::BuildNumber, FirmwareBuildInfo::GitCommit, FirmwareBuildInfo::CompactIdentity);
    ValuePropertyReader valueProperties(valueRuntime_);
    PropertyResolver properties(sensorManager_, measurementSnapshotCache_, actuatorRuntime_, controllerRuntime_, &timeProperties, &valueProperties);
    const bool loadSavedDisplay = server_.hasArg("loadDisplay");
    const bool previewSubmitted = !loadSavedDisplay && server_.hasArg("preview");
    if (server_.hasArg("page") && server_.arg("page") != "1" && server_.arg("page") != "2") {
        sendResult("Invalid page", "/display", "Choose Page 1 or Page 2.", false); return;
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
        sendResult("Invalid state translations", "/display", "Use at most 32 state translations, each up to 16 UTF-8 bytes without control characters.", false);
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
        previewOptions += buildPropertySourceOption(reference, description, name, String());
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
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "version"), "Firmware version");
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "build"), "Build number");
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "git_commit"), "Git commit");
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "build_identity"), "Full build identity");
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "date"), "Date (locale)");
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "time"), "Time (locale)");
    addPreviewSource(PropertyReference(PropertyComponentKind::System, 1, "datetime"), "Date and time (locale)");
    content += buildPropertyPagePreviewHtml(properties, previewOptions, previewPage, loadSavedDisplay || previewSubmitted || configuration.display.configured, pageIndex);
    sendPage("Display", "/display", content);
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
            + "' data-requires='"
            + String(static_cast<unsigned>(metadata->requiredGpioCapabilities))
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
    c = "<section class='card'><h2>Configure Slot " + String(slot.slotId) + "</h2><form method='post' action='/sensors/save'><input type='hidden' name='slot' value='" + String(slot.slotId) + "'>";
    c += "<label class='choice'><input type='checkbox' name='enabled' value='1'" + String(slot.enabled ? " checked" : "") + ">Enabled</label>";
    c += "<label>Name<input name='name' maxlength='" + String(MaxSensorSlotNameLength) + "' required value='" + escapeHtml(slot.name) + "'></label>";
    c += "<label>Implementation<select id='sensorImplementation' name='implementation'>" + options + "</select></label>";
    c += "<div id='gpioConfiguration'><label>GPIO<select name='gpio'>" + gpioOptions + "</select></label><p class='help'>AM2302 uses a custom single-wire protocol. Rain Gauge uses a digital interrupt.</p></div>";
    c += "<div id='i2cConfiguration'><label>Bus<select name='i2cBus'>" + i2cBusOptions + "</select></label><label>I²C address<select id='i2cAddress' name='i2cAddress'><option value='68'" + String(slot.hardware.kind == HardwareResourceKind::I2C && slot.hardware.i2c.address == 0x44 ? " selected" : "") + ">0x44</option><option value='112'" + String(slot.hardware.kind == HardwareResourceKind::I2C && slot.hardware.i2c.address == 0x70 ? " selected" : "") + ">0x70</option><option value='118'" + String(slot.hardware.kind == HardwareResourceKind::I2C && slot.hardware.i2c.address == 0x76 ? " selected" : "") + ">0x76</option><option value='119'" + String(slot.hardware.kind == HardwareResourceKind::I2C && slot.hardware.i2c.address == 0x77 ? " selected" : "") + ">0x77</option></select></label></div>";
    c += "<div id='rainGaugeConfiguration'><label>Millimetres per tip<input type='number' name='millimetersPerTip' min='0.0001' max='100' step='0.0001' value='" + String(slot.implementationConfiguration.rainGauge.millimetersPerTip, 4) + "'></label><label>Debounce time (ms)<input type='number' name='debounceMs' min='1' max='5000' value='" + String(slot.implementationConfiguration.rainGauge.debounceMs) + "'></label></div>";
    c += "<label>Sample interval (ms)<input id='sensorInterval' type='number' min='1' max='2147483647' name='interval' value='" + String(slot.schedule.sampleIntervalMs) + "'></label>";
    c += "<div class='actions'><button type='submit'>Save Slot</button><a class='button' href='/sensors'>Cancel</a></div></form></section>";
    if (selected != nullptr) {
        c += "<section class='card'><h2>Implementation metadata</h2><div class='kv'><span>Type</span><span>" + escapeHtml(selected->displayType) + "</span><span>Interface</span><span>" + hardwareInterfaceKindName(selected->interfaceKind) + " / " + escapeHtml(selected->protocolDescription) + "</span><span>Provenance</span><span>" + String(selected->provenance == SensorProvenance::Simulated ? "Simulated" : "Physical") + "</span><span>Measurements</span><span>" + implementationMeasurements(selected) + "</span></div></section>";
    }
    c += "<script>function sensorFields(reset){const s=document.getElementById('sensorImplementation');const o=s.options[s.selectedIndex];document.getElementById('gpioConfiguration').style.display=o.dataset.interface==='GPIO'?'block':'none';document.getElementById('i2cConfiguration').style.display=o.dataset.interface==='I2C'?'block':'none';document.getElementById('rainGaugeConfiguration').style.display=o.dataset.kind==='rain_gauge'?'block':'none';const g=document.querySelector('[name=gpio]');const r=Number(o.dataset.requires);for(const x of g.options)x.hidden=(Number(x.dataset.capabilities)&r)!==r;if(reset&&o.dataset.interface==='GPIO'&&(g.selectedOptions.length===0||g.selectedOptions[0].hidden)){const x=Array.from(g.options).find(x=>!x.hidden);if(x)g.value=x.value}const a=document.getElementById('i2cAddress');for(const x of a.options)x.hidden=o.dataset.kind==='sht4x'?x.value!=='68':o.dataset.kind==='shtc3'?x.value!=='112':o.dataset.kind==='bme280'?(x.value!=='118'&&x.value!=='119'):false;if(reset&&o.dataset.kind==='sht4x')a.value='68';if(reset&&o.dataset.kind==='shtc3')a.value='112';if(reset&&o.dataset.kind==='bme280'&&(a.value==='68'||a.value==='112'))a.value='118';const n=Number(o.dataset.interval);const f=document.getElementById('sensorInterval');f.parentElement.style.display=n>0?'block':'none';f.disabled=n<=0;if(reset)f.value=n}document.getElementById('sensorImplementation').addEventListener('change',()=>sensorFields(true));sensorFields(false);</script>";
    sendPage("Configure Sensor Slot", "/sensors", c);
}

void WebService::handleActuatorEdit() {
    const long requestedSlot = server_.arg("slot").toInt();
    if (requestedSlot < 1 || requestedSlot > static_cast<long>(MaxActuatorSlotCount)) {
        sendResult("Invalid Actuator Slot", "/actuators",
            "The requested Slot does not exist.", false);
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
        String c = "<section class='card'><h2>Configure Module On/Off Actuator</h2>";
        c += "<p>" + escapeHtml(moduleActuatorLabel(moduleDiscoveryService_, reference)) + "</p>";
        if (hasRuntime) {
            c += "<p>Module " + escapeHtml(moduleSlotName(runtime.moduleSlot)) + " / "
                + escapeHtml(runtime.descriptorDeviceId) + " · "
                + configuredHardwareAssignment(runtime.hardware) + "</p>";
        } else {
            c += "<p>Module actuator is disabled or currently unavailable. Its saved identity is retained.</p>";
        }
        c += "<p class='help'>On/Off capability and hardware are defined by the module descriptor. Changes become active after Apply Actuator Changes.</p>";
        if (module) {
            c += "<form method='post' action='/actuators/save'><input type='hidden' name='slot' value='"
                + String(slot.slotId) + "'><input type='hidden' name='module' value='"
                + moduleActuatorToken(reference) + "'>";
            c += "<label class='choice'><input type='checkbox' name='enabled' value='1'"
                + String((savedModule ? slot.enabled : true) ? " checked" : "") + ">Enabled</label>";
            c += "<label>Name<input name='name' required maxlength='" + String(MaxActuatorSlotNameLength)
                + "' value='" + escapeHtml(savedModule ? slot.name : String(runtime.name)) + "'></label>";
            c += "<div class='actions'><button type='submit'>Save Module Actuator</button><a class='button' href='/actuators'>Cancel</a></div></form>";
        } else {
            c += "<p>A module instance identity is required to save settings.</p>";
        }
        c += "</section>";
        sendPage("Configure Module Actuator", "/actuators", c);
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
        options += ">" + escapeHtml(metadata->displayType) + "</option>";
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
    c = "<section class='card'><h2>Configure Slot " + String(slot.slotId)
        + "</h2><p class='help'>Saved changes become active after applying the actuator composition.</p><form method='post' action='/actuators/save'><input type='hidden' name='slot' value='"
        + String(slot.slotId) + "'>";
    c += "<label class='choice'><input type='checkbox' name='enabled' value='1'"
        + String(slot.enabled ? " checked" : "") + ">Enabled</label>";
    c += "<label>Name<input name='name' maxlength='"
        + String(MaxActuatorSlotNameLength) + "' required value='"
        + escapeHtml(slot.name) + "'></label>";
    c += "<label>Implementation<select id='actuatorImplementation' name='implementation'>"
        + options + "</select></label>";
    c += "<div id='actuatorGpioConfiguration'><label>GPIO<select name='gpio'>"
        + gpioOptions + "</select></label></div>";
    c += "<div class='actions'><button type='submit'>Save Slot</button><a class='button' href='/actuators'>Cancel</a></div></form></section>";
    if (selected != nullptr) {
        c += "<section class='card'><h2>Implementation metadata</h2><div class='kv'><span>Type</span><span>"
            + escapeHtml(selected->displayType) + "</span><span>Interface</span><span>"
            + hardwareInterfaceKindName(selected->interfaceKind) + " / "
            + escapeHtml(selected->protocolDescription)
            + "</span><span>Capability</span><span>"
            + String(hasActuatorCapability(selected->capabilities, ActuatorCapability::Level)
                ? "On/Off, Level"
                : hasActuatorCapability(selected->capabilities, ActuatorCapability::OnOff)
                    ? "On/Off" : "None") + "</span></div></section>";
    }
    c += "<script>function actuatorFields(reset){const s=document.getElementById('actuatorImplementation');const o=s.options[s.selectedIndex];const p=document.getElementById('actuatorGpioConfiguration');p.style.display=o.dataset.interface==='GPIO'?'block':'none';const g=document.querySelector('[name=gpio]');const r=Number(o.dataset.requires);for(const x of g.options)x.hidden=(Number(x.dataset.capabilities)&r)!==r;if(reset&&o.dataset.interface==='GPIO'&&(g.selectedOptions.length===0||g.selectedOptions[0].hidden)){const x=Array.from(g.options).find(x=>!x.hidden);if(x)g.value=x.value}}document.getElementById('actuatorImplementation').addEventListener('change',()=>actuatorFields(true));actuatorFields(false);</script>";
    sendPage("Configure Actuator Slot", "/actuators", c);
}

void WebService::handleControllerEdit() {
    const long requestedSlot = server_.arg("slot").toInt();
    if (requestedSlot < 1 || requestedSlot > static_cast<long>(MaxControllerSlotCount)) {
        sendResult("Invalid Controller Slot", "/controllers",
            "The requested Slot does not exist.", false);
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
        implementationOptions += ">" + escapeHtml(metadata->displayType) + "</option>";
    }
    String blinkTargetOptions;
    String thresholdTargetOptions;
    bool moduleTargetListed = false;
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        const ActuatorSlotConfiguration& actuator = configuration.actuatorSlots[index];
        if (isControllerTargetClaimedByOtherEnabledSlot(
                configuration.controllerSlots,
                MaxControllerSlotCount,
                slot.slotId,
                actuator.slotId)) {
            continue;
        }
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
        blinkTargetOptions += ">Actuator " + String(actuator.slotId) + " · "
            + escapeHtml(actuator.name);
        if (!actuator.enabled) blinkTargetOptions += " (disabled)";
        blinkTargetOptions += "</option>";

        if (isEligibleThresholdActuator(actuator)) {
            thresholdTargetOptions += "<option value='" + String(actuator.slotId) + "'";
            if (slot.implementationConfiguration.threshold.targetActuatorId
                == actuator.slotId) {
                thresholdTargetOptions += " selected";
            }
            thresholdTargetOptions += ">Actuator Slot " + String(actuator.slotId)
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
        const String value = "m:" + String(runtime.id);
        const String label = "Module " + String(moduleSlotName(runtime.moduleSlot))
            + " · " + String(runtime.name);
        blinkTargetOptions += "<option value='" + value + "'";
        if (sameModuleActuatorReference(
                slot.moduleTarget, reference)) {
            blinkTargetOptions += " selected";
            moduleTargetListed = true;
        }
        blinkTargetOptions += ">" + escapeHtml(label) + "</option>";
        thresholdTargetOptions += "<option value='" + value + "'";
        if (sameModuleActuatorReference(
                slot.moduleTarget, reference)) {
            thresholdTargetOptions += " selected";
            moduleTargetListed = true;
        }
        thresholdTargetOptions += ">" + escapeHtml(label) + "</option>";
    }
    if (validModuleActuatorReference(slot.moduleTarget) && !moduleTargetListed) {
        const String unavailable =
            "<option value='m:current' selected>Configured module actuator (currently unavailable)</option>";
        blinkTargetOptions += unavailable;
        thresholdTargetOptions += unavailable;
    }
    if (blinkTargetOptions.isEmpty()) {
        blinkTargetOptions = "<option value='0'>No compatible On/Off actuator configured</option>";
    }
    if (thresholdTargetOptions.isEmpty()) {
        thresholdTargetOptions = "<option value='0'>No enabled On/Off actuator configured</option>";
    }

    const BlinkControllerConfiguration& blink = slot.implementationConfiguration.blink;
    const ThresholdControllerConfiguration& threshold =
        slot.implementationConfiguration.threshold;
    String sourceOptions = "<option value='0'>Select a compatible Sensor</option>";
    String measurementOptions =
        "<option value='unknown' data-unit=''>Select a Measurement</option>";
    String measurementCatalog = "{";
    bool firstCatalogSensor = true;
    for (size_t sensorIndex = 0; sensorIndex < MaxSensorSlotCount; ++sensorIndex) {
        const SensorSlotConfiguration& sensor = configuration.sensorSlots[sensorIndex];
        if (!isEligibleThresholdSensor(sensor)) continue;
        sourceOptions += "<option value='" + String(sensor.slotId) + "'";
        if (threshold.source.sensorId == sensor.slotId) sourceOptions += " selected";
        sourceOptions += ">Sensor Slot " + String(sensor.slotId) + " · "
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
                + "',label:'" + escapeHtml(typeMetadata.displayName)
                + "',unit:'" + escapeHtml(unit == nullptr ? "" : unit) + "'}";
            if (threshold.source.sensorId == sensor.slotId) {
                measurementOptions += "<option value='" + String(typeMetadata.stableId)
                    + "' data-unit='" + escapeHtml(unit == nullptr ? "" : unit) + "'";
                if (threshold.source.measurementType == type) {
                    measurementOptions += " selected";
                }
                measurementOptions += ">" + escapeHtml(typeMetadata.displayName)
                    + "</option>";
            }
        }
        measurementCatalog += "]";
    }
    measurementCatalog += "}";
    String c;
    c.reserve(4200 + sourceOptions.length() + measurementOptions.length());
    c = "<section class='card'><h2>Configure Slot " + String(slot.slotId)
        + "</h2><p class='help'>Saved changes become active after applying the Controller composition.</p><form method='post' action='/controllers/save'><input type='hidden' name='slot' value='"
        + String(slot.slotId) + "'>";
    c += "<label class='choice'><input type='checkbox' name='enabled' value='1'"
        + String(slot.enabled ? " checked" : "") + ">Enabled</label>";
    c += "<label>Name<input name='name' maxlength='" + String(MaxControllerSlotNameLength)
        + "' required value='" + escapeHtml(slot.name) + "'></label>";
    c += "<label>Implementation<select id='controllerImplementation' name='implementation'>"
        + implementationOptions + "</select></label>";
    c += "<div id='blinkConfiguration'><label>Target On/Off actuator<select name='targetActuator'>"
        + blinkTargetOptions + "</select></label>";
    c += "<label>On duration (ms)<input type='number' name='onDuration' min='1' max='2147483647' value='"
        + String(blink.onDurationMs) + "'></label>";
    c += "<label>Off duration (ms)<input type='number' name='offDuration' min='1' max='2147483647' value='"
        + String(blink.offDurationMs) + "'></label></div>";
    c += "<div id='thresholdConfiguration'><label>Source Sensor<select id='thresholdSourceSensor' name='thresholdSourceSensor'>"
        + sourceOptions + "</select></label>";
    c += "<label>Measurement<select id='thresholdMeasurement' name='thresholdMeasurement'>"
        + measurementOptions + "</select></label>";
    c += "<p class='help'>Threshold values use the canonical Measurement unit: <span id='thresholdUnit'></span>.</p>";
    c += "<label class='choice'><input id='thresholdDecisionOnly' type='checkbox' name='decisionOnly' value='1'"
        + String(threshold.decisionOnly ? " checked" : "") + ">Decision only (no actuator control)</label>";
    c += "<label id='thresholdTargetField'>Target On/Off actuator<select name='thresholdTargetActuator'>"
        + thresholdTargetOptions + "</select></label>";
    c += "<label>Switching direction<select name='thresholdDirection'><option value='on_above'"
        + String(threshold.direction == ThresholdDirection::OnAbove ? " selected" : "")
        + ">On at or above the On threshold</option><option value='on_below'"
        + String(threshold.direction == ThresholdDirection::OnBelow ? " selected" : "")
        + ">On at or below the On threshold</option></select></label>";
    c += "<label>On threshold<input type='number' step='any' name='onThreshold' value='"
        + String(threshold.onThreshold, 6) + "'></label>";
    c += "<label>Off threshold<input type='number' step='any' name='offThreshold' value='"
        + String(threshold.offThreshold, 6) + "'></label>";
    c += "<label>Maximum Measurement age (ms)<input type='number' min='1' max='2147483647' name='maxMeasurementAge' value='"
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
    c += "<div class='actions'><button type='submit'>Save Slot</button><a class='button' href='/controllers'>Cancel</a></div></form></section>";
    if (selected != nullptr) {
        c += "<section class='card'><h2>Implementation metadata</h2><div class='kv'><span>Type</span><span>"
            + escapeHtml(selected->displayType) + "</span><span>Required actuator capability</span><span>"
            + String(slot.implementation == ControllerImplementation::Threshold && threshold.decisionOnly
                ? "None" : hasActuatorCapability(selected->requiredActuatorCapabilities,
                ActuatorCapability::OnOff) ? "On/Off" : "None")
            + "</span><span>Description</span><span>" + escapeHtml(selected->description)
            + "</span></div></section>";
    }
    c += "<script>const thresholdMeasurementCatalog=" + measurementCatalog + ";function thresholdMeasurements(rebuild){const s=document.getElementById('thresholdSourceSensor'),m=document.getElementById('thresholdMeasurement'),previous=m.value;if(rebuild){m.replaceChildren(new Option('Select a Measurement','unknown'));for(const x of thresholdMeasurementCatalog[s.value]||[]){const o=new Option(x.label,x.value);o.dataset.unit=x.unit;m.add(o)}if(Array.from(m.options).some(o=>o.value===previous))m.value=previous;else m.value='unknown'}const o=m.options[m.selectedIndex];document.getElementById('thresholdUnit').textContent=o?o.dataset.unit||'':''}function controllerFields(){const s=document.getElementById('controllerImplementation'),k=s.options[s.selectedIndex].dataset.kind;document.getElementById('blinkConfiguration').style.display=k==='blink'?'block':'none';document.getElementById('thresholdConfiguration').style.display=k==='threshold'?'block':'none';document.getElementById('selectorConfiguration').style.display=k==='selector'?'block':'none';document.getElementById('thresholdTargetField').style.display=document.getElementById('thresholdDecisionOnly').checked?'none':'block';thresholdMeasurements(false)}document.getElementById('controllerImplementation').addEventListener('change',controllerFields);document.getElementById('thresholdSourceSensor').addEventListener('change',()=>thresholdMeasurements(true));document.getElementById('thresholdMeasurement').addEventListener('change',()=>thresholdMeasurements(false));document.getElementById('thresholdDecisionOnly').addEventListener('change',controllerFields);controllerFields();</script>";
    sendPage("Configure Controller Slot", "/controllers", c);
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
    c = "<div class='grid'><section class='card'><h2>System</h2><div class='kv'><span>Uptime</span><span>"+localeFormatter_.formatNumber(millis()/1000UL,0)+" s</span><span>Free heap</span><span>"+localeFormatter_.formatNumber(ESP.getFreeHeap(),0)+" bytes</span></div></section><section class='card'><h2>Hardware</h2><div class='kv'><span>Board</span><span>" + escapeHtml(board.displayName) + "</span><span>Board revision</span><span>" + String(board.revision.major) + "." + String(board.revision.minor) + "</span><span>MCU</span><span>" + escapeHtml(ESP.getChipModel()) + "</span><span>CPU frequency</span><span>" + localeFormatter_.formatNumber(ESP.getCpuFreqMHz(), 0) + " MHz</span><span>Flash size</span><span>"+localeFormatter_.formatNumber(ESP.getFlashChipSize(),0)+" bytes</span></div></section><section class='card'><h2>Services</h2><div class='kv'><span>WiFi</span><span>"+(wifiService_.connected()?"Connected":"Disconnected")+"</span><span>MQTT</span><span>"+(mqttService_.connected()?"Connected":"Disconnected")+"</span><span>Time</span><span>"+(timeService_.synchronized()?"Synchronized":"Pending")+"</span></div></section></div>";
    c += "<section class='card'><h2>I²C Diagnostics</h2>";
    if (board.i2cBusCount == 0) {
        c += "<p>No I²C buses available on this board.</p>";
    } else {
        c += "<p class='help'>Showing the latest scan, performed automatically at boot or refreshed manually. Scans probe normal 7-bit addresses for acknowledgement. Module descriptions come from the last EEPROM discovery; scanning does not reload descriptors.</p>";
        for (size_t index = 0; index < board.i2cBusCount; ++index) {
            const BoardI2CBusCapability& bus = board.i2cBuses[index];
            c += "<h3>" + String(i2cBusName(bus.bus)) + "</h3><p class='secondary'>SDA GPIO"
                + String(bus.sda.number) + " · SCL GPIO" + String(bus.scl.number) + "</p>";
            const I2CScanResult* cachedScan = i2cBusManager_.lastScan(bus.bus);
            if (cachedScan == nullptr) {
                c += "<p>Not scanned.</p>";
                continue;
            }
            const I2CScanResult& result = *cachedScan;
            if (result.status == I2CScanStatus::BusUnavailable) {
                c += "<div class='notice error'>Bus is not initialized.</div>";
                continue;
            }
            if (result.addressCount == 0) {
                c += "<p>No devices detected.</p>";
            } else {
                c += "<div class='table-scroll'><table><thead><tr><th>Address</th><th>Device / Module</th></tr></thead><tbody>";
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
                        c += "Mainboard EEPROM";
                        if (boardIdentityResolution_.source == BoardIdentitySource::EEPROM) {
                            const BoardIdentity& identity = boardIdentityResolution_.identity;
                            c += "<br>" + escapeHtml(board.displayName)
                                + " · Revision " + String(identity.revision.major) + "."
                                + String(identity.revision.minor);
                        }
                        c += "<span class='secondary'>Board identity: "
                            + String(boardIdentityStatusName(boardIdentityResolution_.recordStatus))
                            + " · Source: " + boardIdentitySourceName(boardIdentityResolution_.source)
                            + "</span>";
                    } else if (module != nullptr) {
                        c += "Module EEPROM · Slot " + escapeHtml(moduleSlotName(module->slot));
                        if (module->identified()) {
                            c += "<br>" + escapeHtml(descriptorText(module->descriptorName.empty()
                                ? module->descriptorTypeId : module->descriptorName))
                                + " · Revision " + String(module->descriptorRevision.major) + "."
                                + String(module->descriptorRevision.minor);
                            c += "<span class='secondary'>Descriptor: "
                                + String(hardwareDescriptorDecodeStatusName(module->descriptorStatus))
                                + " · " + hardwareDescriptorCompatibilityStatusName(module->descriptorCompatibility)
                                + "</span>";
                        } else {
                            c += "<span class='secondary'>Descriptor: "
                                + String(module->descriptorStoreStatus == HardwareDescriptorStoreStatus::Valid
                                    ? hardwareDescriptorDecodeStatusName(module->descriptorStatus)
                                    : hardwareDescriptorStoreStatusName(module->descriptorStoreStatus)) + "</span>";
                        }
                    } else {
                        c += "Unidentified I²C device";
                    }
                    c += "</td></tr>";
                }
                c += "</tbody></table></div>";
            }
            if (result.status == I2CScanStatus::CompleteWithProbeErrors) {
                c += "<div class='notice error'>The scan encountered "
                    + localeFormatter_.formatNumber(result.probeErrorCount, 0)
                    + " bus probe errors. See logs.</div>";
            }
        }
        c += "<form method='post' action='/diagnostics/i2c/scan'><button type='submit'>Scan I²C buses</button></form>";
    }
    c += "</section>";
    for(size_t index=0;index<sensorManager_.sensorCount();++index){SensorRuntimeInfo i; if(!sensorManager_.runtimeInfo(index,i))continue;SensorRuntimeStatus s;sensorManager_.runtimeStatus(i.id,s);c+="<section class='card'><h2>Sensor "+localeFormatter_.formatNumber(i.id,0)+" · "+escapeHtml(i.name)+"</h2><div class='kv'><span>Type</span><span>"+escapeHtml(i.type)+"</span><span>State</span><span>"+sensorStateName(i.state)+"</span><span>Accepted</span><span>"+localeFormatter_.formatNumber(s.acceptedMeasurementCount,0)+"</span><span>Rejected</span><span>"+localeFormatter_.formatNumber(s.rejectedMeasurementCount,0)+"</span><span>Pre-sync discarded</span><span>"+localeFormatter_.formatNumber(s.preSyncDiscardCount,0)+"</span><span>Last sample emissions</span><span>"+localeFormatter_.formatNumber(s.lastSampleEmissionCount,0)+"</span></div></section>";}
    sendPage("Diagnostics", "/diagnostics", c);
}

void WebService::handleLogs() {
    sendPage("Logs", "/logs", buildRecentLogHtml(logReader_));
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
        sendResult("Board provisioning failed", "/device",
            "The selected board profile or board serial number is invalid.", false);
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
        content = "<div class='notice success'><strong>Board Identity written and verified.</strong><p>The active BoardProfile is unchanged for this boot. Restart is required to select the EEPROM identity.</p></div><form method='post' action='/restart' onsubmit='return confirm(\"Restart the device now?\")'><button>Restart Now</button></form><a class='button' href='/device'>Restart Later</a>";
        sendPage("Board Identity provisioned", "/device", content);
        return;
    }

    const char* message = "Board Identity could not be written and verified.";
    switch (result.status) {
        case BoardProvisioningStatus::ConfirmationRequired:
            message = "Explicit confirmation is required. No EEPROM data was written.";
            break;
        case BoardProvisioningStatus::InvalidIdentity:
            message = "The requested Board Identity is not supported. No EEPROM data was written.";
            break;
        case BoardProvisioningStatus::WriteFailed:
            message = "The EEPROM did not accept the write. It may be absent or unavailable. The active BoardProfile is unchanged.";
            break;
        case BoardProvisioningStatus::ReadbackFailed:
            message = "The EEPROM write could not be read back. Provisioning was not accepted as successful.";
            break;
        case BoardProvisioningStatus::ReadbackInvalid:
            message = "The EEPROM readback failed record validation. Provisioning was not accepted as successful.";
            break;
        case BoardProvisioningStatus::ReadbackMismatch:
            message = "The EEPROM readback did not exactly match the requested identity. Provisioning was not accepted as successful.";
            break;
        default:
            break;
    }
    sendResult("Board provisioning failed", "/device", message, false);
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
        sendResult("Module descriptor provisioning failed", "/device",
            "The slot, descriptor template, or manufacturing data is invalid. No EEPROM data was written.",
            false);
        return;
    }
    esp_fill_random(manufacturing.instanceId, sizeof(manufacturing.instanceId));
    InstanceUuid::makeVersion4(manufacturing.instanceId);
    manufacturing.serialNumber = serialNumber.c_str();
    manufacturing.productionBatch = productionBatch.c_str();
    manufacturing.productionDate = productionDate.c_str();
    size_t payloadSize = 0;
    const CompactCborStatus encodeStatus = DuoRelayDescriptor::encode(
        manufacturing, moduleDescriptorPayload_,
        sizeof(moduleDescriptorPayload_), payloadSize);
    if (encodeStatus != CompactCborStatus::Success) {
        logger_.errorf("DuoRelay descriptor encoding failed status=%s",
            compactCborStatusName(encodeStatus));
        sendResult("Module descriptor provisioning failed", "/device",
            "The DuoRelay descriptor could not be encoded. No EEPROM data was written.",
            false);
        return;
    }

    const bool confirmed = server_.hasArg("confirmModuleDescriptorProvisioning")
        && server_.arg("confirmModuleDescriptorProvisioning") == "1";
    const ModuleDescriptorProvisioningResult result =
        moduleDescriptorProvisioningService_.provision(
            slot, moduleDescriptorPayload_, payloadSize, confirmed);
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
        String message = "The DuoRelay descriptor was encoded, validated, written to Bank ";
        message += bank;
        message += ", verified, and rediscovered with instance UUID ";
        message += descriptorUuid(manufacturing.instanceId);
        message += ". No restart is required.";
        sendResult("Module descriptor provisioned", "/device", message.c_str(), true);
        return;
    }

    const char* message = "The module descriptor could not be written and verified.";
    switch (result.status) {
        case ModuleDescriptorProvisioningStatus::ConfirmationRequired:
            message = "Explicit confirmation is required. No EEPROM data was written.";
            break;
        case ModuleDescriptorProvisioningStatus::InvalidSlot:
            message = "The selected module slot is invalid. No EEPROM data was written.";
            break;
        case ModuleDescriptorProvisioningStatus::InvalidDescriptor:
            message = "The generated descriptor failed semantic validation. No EEPROM data was written.";
            break;
        case ModuleDescriptorProvisioningStatus::IncompatibleDescriptor:
            message = "The descriptor is not compatible with the active board, firmware, drivers, capabilities, or slot resources. No EEPROM data was written.";
            break;
        case ModuleDescriptorProvisioningStatus::StorageUnavailable:
            message = "The selected module EEPROM is unavailable. Check that the module is installed in the selected slot.";
            break;
        case ModuleDescriptorProvisioningStatus::WriteFailed:
            message = "The selected module EEPROM did not accept the descriptor write.";
            break;
        case ModuleDescriptorProvisioningStatus::VerificationFailed:
            message = "The descriptor write failed atomic-bank readback verification.";
            break;
        case ModuleDescriptorProvisioningStatus::RediscoveryFailed:
            message = "The descriptor was written and verified, but semantic rediscovery did not succeed. Inspect diagnostics before retrying.";
            break;
        default:
            break;
    }
    sendResult("Module descriptor provisioning failed", "/device", message, false);
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
                "Module Actuator saved", "Module Actuator save failed", "/actuators",
                "Invalid configuration or module identity changed. Reload the editor.");
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
        "Actuator Slot saved",
        "Actuator Slot save failed",
        "/actuators",
        "Invalid Slot configuration, hardware resource, or resource conflict.");
}
void WebService::handleActuatorOn() { handleActuatorState(OnOffState::On); }
void WebService::handleActuatorOff() { handleActuatorState(OnOffState::Off); }
void WebService::handleActuatorState(OnOffState state) {
    const long requestedSlot = server_.arg("slot").toInt();
    if (requestedSlot < 1 || requestedSlot > static_cast<long>(MaxActuatorSlotCount)) {
        sendResult("Actuator command failed", "/actuators",
            "The requested Actuator Slot does not exist.", false);
        return;
    }
    IOnOffActuator* actuator = actuatorRuntime_.onOffActuator(
        static_cast<ActuatorId>(requestedSlot));
    if (actuator == nullptr) {
        sendResult("Actuator command failed", "/actuators",
            "No initialized On/Off actuator is active for this Slot.", false);
        return;
    }
    const ActuatorOperationResult result = actuator->setState(state);
    if (result != ActuatorOperationResult::Completed) {
        sendResult("Actuator command failed", "/actuators",
            actuatorOperationFailure(result), false);
        return;
    }
    server_.sendHeader("Location", "/actuators", true);
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
        sendResult("Actuator Level command failed", "/actuators",
            "The requested Actuator Slot or Level is invalid.", false);
        return;
    }
    ILevelActuator* actuator = actuatorRuntime_.levelActuator(
        static_cast<ActuatorId>(requestedSlot));
    if (actuator == nullptr) {
        sendResult("Actuator Level command failed", "/actuators",
            "No initialized Level-capable actuator is active for this Slot.", false);
        return;
    }
    ActuatorLevel level = ActuatorLevel::off();
    if (!ActuatorLevel::tryCreate(static_cast<uint8_t>(requestedLevel), level)
        || actuator->setLevel(level) != ActuatorOperationResult::Completed) {
        sendResult("Actuator Level command failed", "/actuators",
            "The Level-capable actuator rejected the operation.", false);
        return;
    }
    server_.sendHeader("Location", "/actuators", true);
    server_.send(303, "text/plain", "See Other");
}
void WebService::handleActuatorApply() {
    if (runtimeManager_.pendingAction() != RuntimeAction::RestartActuatorRuntime) {
        sendResult("Actuator changes not applied", "/actuators",
            "No actuator runtime apply is pending, or a stronger runtime action takes priority.",
            false);
        return;
    }
    if (!runtimeManager_.applyPendingActuatorChanges(
            configurationService_.getConfiguration().actuatorSlots)) {
        sendResult("Actuator changes not applied", "/actuators",
            "The actuator runtime rebuild failed. The previous composition was retained or restored; see logs.",
            false);
        return;
    }
    server_.sendHeader("Location", "/actuators", true);
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
        "Controller Slot saved",
        "Controller Slot save failed",
        "/controllers",
        "Invalid Controller configuration. Select an unclaimed target. Selectors need an enabled decision-only Threshold and three distinct options from the selected Value. Check thresholds and timing values.");
}
void WebService::handleControllerApply() {
    if (runtimeManager_.pendingAction() != RuntimeAction::RestartControllerRuntime) {
        sendResult("Controller changes not applied", "/controllers",
            "No Controller runtime apply is pending, or a stronger runtime action takes priority.",
            false);
        return;
    }
    if (!runtimeManager_.applyPendingControllerChanges(
            configurationService_.getConfiguration().controllerSlots)) {
        sendResult("Controller changes not applied", "/controllers",
            "The Controller runtime rebuild failed. The previous composition was retained or restored; see logs.",
            false);
        return;
    }
    server_.sendHeader("Location", "/controllers", true);
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
        sendResult("Controller command failed", "/controllers",
            "The requested Controller Slot does not exist.", false);
        return;
    }
    const ControllerId id = static_cast<ControllerId>(requestedSlot);
    const ControllerOperationResult result = start
        ? controllerRuntime_.startController(id)
        : controllerRuntime_.stopController(id);
    if (result != ControllerOperationResult::Completed
        && result != ControllerOperationResult::NoAction) {
        String message = start ? "The Controller could not be started: "
            : "The Controller could not be stopped cleanly: ";
        message += controllerOperationName(result);
        message += ".";
        sendResult("Controller command failed", "/controllers", message.c_str(), false);
        return;
    }
    server_.sendHeader("Location", "/controllers", true);
    server_.send(303, "text/plain", "See Other");
}
void WebService::handleRestart() { if(otaService_.busy()){sendResult("Restart unavailable","/firmware","A firmware upload is currently active.",false);return;} runtimeManager_.request(RuntimeAction::RestartDevice); sendResult("Restarting","/firmware","The device is restarting now.",true); performExplicitRestart(); }
void WebService::handleFactoryReset() { if(otaService_.busy()){sendResult("Factory reset unavailable","/firmware","A firmware upload is currently active.",false);return;} if(!configurationService_.resetToDefaults()){sendResult("Factory reset failed","/firmware","Stored configuration could not be cleared.",false);return;} runtimeManager_.request(RuntimeAction::RestartDevice); sendResult("Factory reset complete","/firmware","Configuration erased. Restarting into provisioning mode.",true);performExplicitRestart(); }
void WebService::handleNotFound() { server_.send(404,"text/plain","Not Found"); }
void WebService::performExplicitRestart() { server_.client().flush(); runtimeManager_.performPendingRestart(); }

} // namespace EnvNode
