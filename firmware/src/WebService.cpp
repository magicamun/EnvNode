#include "WebService.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_netif.h>
#include "FirmwareVersion.h"
#include "UnitConverter.h"

namespace WeatherStation {
namespace {

const char SharedStyle[] PROGMEM = R"CSS(
:root{--bg:#f3f6f8;--panel:#fff;--ink:#17212b;--muted:#637282;--line:#dbe3e8;--brand:#176b87;--brand2:#0f536a;--good:#177245;--warn:#a55b00;--bad:#a32828}html,body,*,*::before,*::after{box-sizing:border-box}html,body{max-width:100%}body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.45 system-ui,-apple-system,sans-serif;overflow-x:hidden}.shell{min-height:100vh;min-width:0;display:grid;grid-template-columns:220px minmax(0,1fr)}.side{background:#123644;color:#fff;padding:22px 16px;min-width:0}.brand{font-weight:750;font-size:19px;margin:0 8px 4px;overflow-wrap:anywhere}.version{color:#b8d0da;font-size:12px;margin:0 8px 20px}.nav a{display:block;color:#dbeaf0;text-decoration:none;padding:9px 11px;border-radius:7px;margin:2px 0}.nav a:hover,.nav a.active{background:#1d5367;color:#fff}.main{padding:28px;max-width:1100px;width:100%;min-width:0}.top{display:flex;justify-content:space-between;gap:16px;align-items:start;margin-bottom:22px;min-width:0}h1{font-size:25px;margin:0;overflow-wrap:anywhere}h2{font-size:17px;margin:0 0 14px}p{margin:8px 0;overflow-wrap:anywhere}.muted,.help{color:var(--muted)}.help{font-size:13px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(240px,100%),1fr));gap:16px;min-width:0;max-width:100%}.card{background:var(--panel);border:1px solid var(--line);border-radius:10px;padding:18px;margin-bottom:16px;box-shadow:0 1px 2px #1122;min-width:0;max-width:100%;overflow:hidden}.kv{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1.4fr);gap:8px 14px;min-width:0;max-width:100%}.kv>span{min-width:0;max-width:100%;overflow-wrap:anywhere}.kv span:nth-child(odd){color:var(--muted)}.badge{display:inline-block;border-radius:99px;padding:3px 9px;font-size:12px;font-weight:700;background:#e8edf0;max-width:100%;white-space:normal;overflow-wrap:anywhere}.badge.good{color:var(--good);background:#e2f4ea}.badge.warn{color:var(--warn);background:#fff0d7}.badge.bad{color:var(--bad);background:#fbe3e3}.notice{border-left:4px solid var(--warn);background:#fff8e9;padding:11px 13px;border-radius:5px;margin-bottom:16px;max-width:100%;overflow-wrap:anywhere}.success{border-left-color:var(--good);background:#eaf7ef}.error{border-left-color:var(--bad);background:#fdecec}label{display:block;font-weight:650;margin:0 0 14px;min-width:0;max-width:100%;overflow-wrap:anywhere}input,select{display:block;width:100%;max-width:520px;min-width:0;margin-top:5px;padding:9px 10px;border:1px solid #bfcbd2;border-radius:6px;background:#fff;color:var(--ink);font:inherit}input[type=checkbox],input[type=radio]{display:inline;width:auto;margin:0 7px 0 0}.choice{font-weight:500;margin:7px 0}.actions{display:flex;gap:10px;flex-wrap:wrap;margin-top:18px;min-width:0}button,.button{border:0;border-radius:6px;padding:9px 15px;background:var(--brand);color:#fff;text-decoration:none;font:600 14px inherit;cursor:pointer;max-width:100%;white-space:normal}button:hover,.button:hover{background:var(--brand2)}button.danger{background:var(--bad)}table{width:100%;border-collapse:collapse;font-size:14px}th,td{text-align:left;padding:9px;border-bottom:1px solid var(--line);vertical-align:top;overflow-wrap:anywhere}th{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.03em}.scroll{overflow-x:auto;max-width:100%;min-width:0}details{margin-top:12px;max-width:100%}summary{cursor:pointer;font-weight:650}@media(max-width:760px){.shell{display:block}.side{padding:14px}.brand,.version{display:inline-block;margin:0 8px 10px 0}.nav{display:flex;overflow-x:auto;gap:3px}.nav a{white-space:nowrap}.main{padding:18px 13px}.top{display:block}.kv{grid-template-columns:minmax(0,1fr)}.kv span:nth-child(even){margin-bottom:7px}.card{padding:15px}}
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
    if (info.supportsTemperature) result += "Temperature, ";
    if (info.supportsRelativeHumidity) result += "Relative Humidity, ";
    if (info.supportsAtmosphericPressure) result += "Atmospheric Pressure, ";
    if (info.supportsSolarIrradiance) result += "Solar Irradiance, ";
    if (info.supportsSolarCellTemperature) result += "Solar Cell Temperature, ";
    if (info.supportsRainDetectorLevel) result += "Rain Detector Level, ";
    if (info.supportsRainDetectorWet) result += "Rain Detector Wet, ";
    if (info.supportsRainGaugeTip) result += "Rain Gauge Tip, ";
    if (result.endsWith(", ")) result.remove(result.length() - 2);
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

} // namespace

WebService::WebService(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService,
    IMqttService& mqttService, ITimeService& timeService, LocaleFormatter& localeFormatter,
    SensorManager& sensorManager)
    : logger_(logger), configurationService_(configurationService), wifiService_(wifiService),
      mqttService_(mqttService), timeService_(timeService), localeFormatter_(localeFormatter),
      sensorManager_(sensorManager) {}

void WebService::begin() {
    server_.on("/", HTTP_GET, [this]() { handleStatus(); });
    server_.on("/status", HTTP_GET, [this]() { handleStatus(); });
    server_.on("/sensors", HTTP_GET, [this]() { handleSensors(); });
    server_.on("/network", HTTP_GET, [this]() { handleNetwork(); });
    server_.on("/mqtt", HTTP_GET, [this]() { handleMqtt(); });
    server_.on("/time", HTTP_GET, [this]() { handleTime(); });
    server_.on("/units", HTTP_GET, [this]() { handleUnits(); });
    server_.on("/device", HTTP_GET, [this]() { handleDevice(); });
    server_.on("/diagnostics", HTTP_GET, [this]() { handleDiagnostics(); });
    server_.on("/firmware", HTTP_GET, [this]() { handleFirmware(); });
    server_.on("/style.css", HTTP_GET, [this]() { handleStyle(); });
    server_.on("/network/save", HTTP_POST, [this]() { handleNetworkSave(); });
    server_.on("/mqtt/save", HTTP_POST, [this]() { handleMqttSave(); });
    server_.on("/time/save", HTTP_POST, [this]() { handleTimeSave(); });
    server_.on("/units/save", HTTP_POST, [this]() { handleUnitsSave(); });
    server_.on("/device/save", HTTP_POST, [this]() { handleDeviceSave(); });
    server_.on("/restart", HTTP_POST, [this]() { handleRestart(); });
    server_.on("/factory-reset", HTTP_POST, [this]() { handleFactoryReset(); });
    server_.onNotFound([this]() { handleNotFound(); });
    server_.begin();
    logger_.println("Web administration started");
}

void WebService::loop() {
    server_.handleClient();
    if (restartAtMs_ != 0 && static_cast<int32_t>(millis() - restartAtMs_) >= 0) ESP.restart();
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

String WebService::navigationHtml(const char* active) const {
    const char* routes[][2] = {{"/status","Status"},{"/sensors","Sensors"},{"/network","Network"},{"/mqtt","MQTT"},{"/time","Locale & Time"},{"/units","Units"},{"/device","Device"},{"/diagnostics","Diagnostics"},{"/firmware","Firmware"}};
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
    html += FirmwareVersion;
    html += "</div>";
    html += navigationHtml(active);
    html += "</aside><main class='main'><div class='top'><div><h1>";
    html += escapeHtml(title);
    html += "</h1>";
    if (wifiService_.inSetupAccessPointMode()) html += "<p class='muted'>Setup access point mode</p>";
    html += "</div></div>" + content + "</main></div></body></html>";
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
    c = "<div class='grid'><section class='card'><h2>Device</h2><div class='kv'><span>Name</span><span>" + escapeHtml(cfg.device.name) + "</span><span>Firmware</span><span>" + FirmwareVersion + "</span><span>Uptime</span><span>" + localeFormatter_.formatNumber(millis()/1000UL, 0) + " seconds</span><span>Free heap</span><span>" + localeFormatter_.formatNumber(ESP.getFreeHeap(), 0) + " bytes</span><span>Flash</span><span>" + localeFormatter_.formatNumber(ESP.getFlashChipSize()/1024UL, 0) + " KB</span></div></section>";
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
    c = "<div class='notice'><strong>Restart required for network changes.</strong><p>Saved settings become active after an explicit restart.</p></div><section class='card'><h2>Network configuration</h2><form method='post' action='/network/save' autocomplete='off'>";
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
    c.reserve(900);
    c = "<section class='card'><h2>Runtime status</h2>" + (m.server.isEmpty()?badge("Not configured","warn"):mqttService_.connected()?badge("Connected","good"):badge("Disconnected","bad")) + "</section><section class='card'><h2>Broker configuration</h2><form method='post' action='/mqtt/save' autocomplete='off'>";
    c += "<label>Broker hostname or IP<input name='server' value='" + escapeHtml(m.server) + "'></label><label>Port<input type='number' min='1' max='65535' name='port' required value='" + String(m.port) + "'></label><label>Username<input name='username' value='" + escapeHtml(m.username) + "'></label><label class='choice'><input type='checkbox' name='changePassword' value='1'>Change MQTT password</label><label>New password<input type='password' name='password' autocomplete='new-password'></label><div class='actions'><button>Save MQTT settings</button></div></form></section>";
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
    c.reserve(650 + sensorManager_.sensorCount() * 300);
    c = "<div class='notice'><strong>Read-only milestone</strong><p>Sensor implementation selection, hardware detection, enablement, and persistent Slot configuration require the future SensorFactory and Sensor Slot backend.</p></div><section class='card'><h2>Registered runtime sensors</h2><div class='scroll'><table><thead><tr><th>Slot / ID</th><th>Enabled</th><th>Implementation</th><th>Detected</th><th>Provenance</th><th>State</th><th>Schedule</th><th>Measurements</th><th>Configure</th></tr></thead><tbody>";
    for (size_t index=0; index<sensorManager_.sensorCount(); ++index) { SensorRuntimeInfo i; if (!sensorManager_.runtimeInfo(index,i)) continue; c += "<tr><td>"+localeFormatter_.formatNumber(i.id,0)+"</td><td>"+(i.schedule.enabled?"Yes":"No")+"</td><td>Not exposed</td><td>Not exposed</td><td>"+(i.provenance==SensorProvenance::Simulated?"Simulated":"Physical")+"</td><td>"+String(sensorStateName(i.state))+"</td><td>"+(i.schedule.acquisitionMode==AcquisitionMode::Periodic?localeFormatter_.formatNumber(i.schedule.sampleIntervalMs,0)+" ms":"Event only")+"</td><td>"+measurementTypes(i)+"</td><td>Future</td></tr>"; }
    c += "</tbody></table></div></section>";
    sendPage("Sensors", "/sensors", c);
}

void WebService::handleDiagnostics() {
    String c;
    c.reserve(750 + sensorManager_.sensorCount() * 450);
    c = "<div class='grid'><section class='card'><h2>System</h2><div class='kv'><span>Uptime</span><span>"+localeFormatter_.formatNumber(millis()/1000UL,0)+" s</span><span>Free heap</span><span>"+localeFormatter_.formatNumber(ESP.getFreeHeap(),0)+" bytes</span><span>Flash</span><span>"+localeFormatter_.formatNumber(ESP.getFlashChipSize(),0)+" bytes</span></div></section><section class='card'><h2>Services</h2><div class='kv'><span>WiFi</span><span>"+(wifiService_.connected()?"Connected":"Disconnected")+"</span><span>MQTT</span><span>"+(mqttService_.connected()?"Connected":"Disconnected")+"</span><span>Time</span><span>"+(timeService_.synchronized()?"Synchronized":"Pending")+"</span></div></section></div>";
    for(size_t index=0;index<sensorManager_.sensorCount();++index){SensorRuntimeInfo i; if(!sensorManager_.runtimeInfo(index,i))continue;SensorRuntimeStatus s;sensorManager_.runtimeStatus(i.id,s);c+="<section class='card'><h2>Sensor "+localeFormatter_.formatNumber(i.id,0)+"</h2><div class='kv'><span>State</span><span>"+sensorStateName(i.state)+"</span><span>Accepted</span><span>"+localeFormatter_.formatNumber(s.acceptedMeasurementCount,0)+"</span><span>Rejected</span><span>"+localeFormatter_.formatNumber(s.rejectedMeasurementCount,0)+"</span><span>Pre-sync discarded</span><span>"+localeFormatter_.formatNumber(s.preSyncDiscardCount,0)+"</span><span>Last sample emissions</span><span>"+localeFormatter_.formatNumber(s.lastSampleEmissionCount,0)+"</span></div></section>";}
    sendPage("Diagnostics", "/diagnostics", c);
}

void WebService::handleFirmware() {
    String c;
    c.reserve(1100);
    c = "<section class='card'><h2>Firmware</h2><div class='kv'><span>Version</span><span>"+String(FirmwareVersion)+"</span></div><p class='help'>Firmware upload and OTA will be added in a later milestone.</p></section><section class='card'><h2>Restart</h2><form method='post' action='/restart' onsubmit='return confirm(\"Restart the device now?\")'><button>Restart device</button></form></section><section class='card'><h2>Factory reset</h2><p>This permanently removes all stored configuration and restarts into provisioning mode.</p><form method='post' action='/factory-reset' onsubmit='return confirm(\"Erase ALL configuration and restart?\")'><button class='danger'>Factory reset</button></form></section>";
    sendPage("Firmware", "/firmware", c);
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
    if(!configurationService_.setNetworkConfiguration(n,updatePassword)){sendResult("Network save failed","/network","Invalid network settings. Static mode requires a valid address, contiguous subnet mask, same-subnet gateway, and primary DNS.",false);return;}
    sendResult("Network settings saved","/network","Restart required for network changes. Use the Firmware page when ready.",true);
}

void WebService::handleMqttSave() {
    const String portText=server_.arg("port"); const long port=portText.toInt();
    bool ok=port>=1&&port<=65535&&configurationService_.setMqttServer(server_.arg("server"))&&configurationService_.setMqttPort(static_cast<uint16_t>(port))&&configurationService_.setMqttUsername(server_.arg("username"));
    if(ok&&server_.arg("changePassword")=="1")ok=configurationService_.setMqttPassword(server_.arg("password"));
    sendResult(ok?"MQTT settings saved":"MQTT save failed","/mqtt",ok?"Configuration saved. The service will use it on its next connection attempt.":"Invalid MQTT configuration.",ok);
}

void WebService::handleTimeSave() { Locale locale; bool ok=parseLocaleKey(server_.arg("locale").c_str(),locale); if(ok)ok=configurationService_.setLocale(locale)&&configurationService_.setTimezone(server_.arg("timezone"))&&configurationService_.setNtpServer1(server_.arg("ntpServer1"))&&configurationService_.setNtpServer2(server_.arg("ntpServer2")); sendResult(ok?"Locale and time saved":"Locale and time save failed","/time",ok?"Configuration saved. Locale applies to subsequent page rendering.":"Invalid locale or time configuration.",ok); }

void WebService::handleUnitsSave() {
    PresentationUnit t,p,s,r; bool ok=parseUnit(server_.arg("temperature"),MeasurementType::Temperature,t)&&parseUnit(server_.arg("pressure"),MeasurementType::AtmosphericPressure,p)&&parseUnit(server_.arg("solarTemperature"),MeasurementType::SolarCellTemperature,s)&&parseUnit(server_.arg("rainLevel"),MeasurementType::RainDetectorLevel,r);
    if(ok)ok=configurationService_.setPresentationUnit(MeasurementType::Temperature,t)&&configurationService_.setPresentationUnit(MeasurementType::AtmosphericPressure,p)&&configurationService_.setPresentationUnit(MeasurementType::SolarCellTemperature,s)&&configurationService_.setPresentationUnit(MeasurementType::RainDetectorLevel,r);
    sendResult(ok?"Presentation units saved":"Unit save failed","/units",ok?"Future measurements use the selected presentation units immediately.":"Invalid or unsupported presentation unit.",ok);
}

void WebService::handleDeviceSave() { const bool ok=configurationService_.setDeviceName(server_.arg("deviceName")); sendResult(ok?"Device settings saved":"Device save failed","/device",ok?"Device name saved.":"Invalid device name.",ok); }
void WebService::handleRestart() { sendResult("Restarting","/firmware","The device is restarting now.",true); scheduleRestart(); }
void WebService::handleFactoryReset() { if(!configurationService_.resetToDefaults()){sendResult("Factory reset failed","/firmware","Stored configuration could not be cleared.",false);return;} sendResult("Factory reset complete","/firmware","Configuration erased. Restarting into provisioning mode.",true);scheduleRestart(); }
void WebService::handleNotFound() { server_.send(404,"text/plain","Not Found"); }
void WebService::scheduleRestart() { if(restartAtMs_==0){restartAtMs_=millis()+1000;logger_.println("Restart scheduled by Web administration");} }

} // namespace WeatherStation
