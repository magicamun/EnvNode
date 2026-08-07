#include "WebService.h"
#include "UnitConverter.h"
#include <Arduino.h>
#include <WiFi.h>

namespace WeatherStation {

namespace {
struct TimezoneOption {
    const char* label;
    const char* value;
};

constexpr TimezoneOption TimezoneOptions[] = {
    {"Europe/Berlin", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"UTC", "UTC0"},
    {"America/New_York", "EST5EDT,M3.2.0/2,M11.1.0/2"},
    {"America/Chicago", "CST6CDT,M3.2.0/2,M11.1.0/2"},
    {"America/Denver", "MST7MDT,M3.2.0/2,M11.1.0/2"},
    {"America/Los_Angeles", "PST8PDT,M3.2.0/2,M11.1.0/2"},
};

String timezoneSelectHtml(const String& currentTimezone) {
    String html;
    bool customSelected = true;
    html += "<select name='timezone'>";

    for (const auto& option : TimezoneOptions) {
        const String value(option.value);
        const bool selected = (!currentTimezone.isEmpty() && currentTimezone == value);
        if (selected) {
            customSelected = false;
        }
        html += "<option value='" + value + "'";
        if (selected) {
            html += " selected";
        }
        html += ">";
        html += option.label;
        html += "</option>";
    }

    if (customSelected && !currentTimezone.isEmpty()) {
        html += "<option value='" + currentTimezone + "' selected>Custom: " + currentTimezone + "</option>";
    }

    html += "</select>";
    return html;
}

String presentationUnitSelectHtml(
    const char* fieldName,
    MeasurementType type,
    PresentationUnit currentUnit) {
    const MeasurementTypeMetadata& metadata = measurementTypeMetadata(type);
    String html = "<select name='" + String(fieldName) + "'>";
    for (uint8_t index = 0; index < metadata.supportedPresentationUnitCount; ++index) {
        const PresentationUnit unit = metadata.supportedPresentationUnits[index];
        html += "<option value='";
        html += UnitConverter::stableKey(unit);
        html += "'";
        if (unit == currentUnit) {
            html += " selected";
        }
        html += ">";
        html += UnitConverter::displayName(unit);
        html += " (";
        html += UnitConverter::symbol(unit);
        html += ")</option>";
    }
    html += "</select>";
    return html;
}

bool parsePresentationUnitArgument(
    const String& argument,
    MeasurementType type,
    PresentationUnit& unit) {
    return UnitConverter::parseStableKey(argument.c_str(), unit)
        && supportsPresentationUnit(type, unit);
}
}

WebService::WebService(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService)
    : logger_(logger)
    , configurationService_(configurationService)
    , wifiService_(wifiService) {
}

void WebService::begin() {
    server_.on("/", HTTP_GET, [this]() { handleRoot(); });
    server_.on("/save", HTTP_POST, [this]() { handleSave(); });
    server_.on("/reset", HTTP_POST, [this]() { handleReset(); });
    server_.onNotFound([this]() { handleNotFound(); });
    server_.begin();
    logger_.println("Web service started");
}

void WebService::loop() {
    server_.handleClient();

    if (restartAtMs_ != 0 && millis() >= restartAtMs_) {
        logger_.println("Restarting after configuration save");
        ESP.restart();
    }
}

void WebService::handleRoot() {
    if (!isProvisioningEnabled()) {
        server_.send(404, "text/plain", "Not Found");
        return;
    }

    logger_.println("Provisioning page requested");
    server_.send(200, "text/html", configurationPage());
}

void WebService::handleSave() {
    if (!isProvisioningEnabled()) {
        server_.send(404, "text/plain", "Not Found");
        return;
    }

    const String deviceName = server_.arg("deviceName");
    const String wifiSSID = server_.arg("wifiSSID");
    const String wifiPassword = server_.arg("wifiPassword");
    const String changeWifiPassword = server_.arg("changeWifiPassword");
    const String mqttServer = server_.arg("mqttServer");
    const String mqttPortStr = server_.arg("mqttPort");
    uint16_t mqttPort = 0;
    if (!mqttPortStr.isEmpty()) {
        mqttPort = static_cast<uint16_t>(mqttPortStr.toInt());
    }
    const String mqttUsername = server_.arg("mqttUsername");
    const String mqttPassword = server_.arg("mqttPassword");
    const String changeMqttPassword = server_.arg("changeMqttPassword");
    const String timezone = server_.arg("timezone");
    const String ntpServer1 = server_.arg("ntpServer1");
    const String ntpServer2 = server_.arg("ntpServer2");
    PresentationUnit temperatureUnit = PresentationUnit::None;
    PresentationUnit pressureUnit = PresentationUnit::None;
    PresentationUnit solarCellTemperatureUnit = PresentationUnit::None;
    PresentationUnit rainDetectorLevelUnit = PresentationUnit::None;

    bool ok = true;
    String errorMessage;

    if (!parsePresentationUnitArgument(
            server_.arg("temperatureUnit"), MeasurementType::Temperature, temperatureUnit)
        || !parsePresentationUnitArgument(
            server_.arg("pressureUnit"), MeasurementType::AtmosphericPressure, pressureUnit)
        || !parsePresentationUnitArgument(
            server_.arg("solarCellTemperatureUnit"), MeasurementType::SolarCellTemperature,
            solarCellTemperatureUnit)
        || !parsePresentationUnitArgument(
            server_.arg("rainDetectorLevelUnit"), MeasurementType::RainDetectorLevel,
            rainDetectorLevelUnit)) {
        logger_.println("Config failed: unsupported presentation unit");
        ok = false;
        errorMessage = "Invalid or unsupported presentation unit.";
    }

    if (ok && !configurationService_.setDeviceName(deviceName)) {
        logger_.println("Config failed: deviceName");
        ok = false;
        errorMessage = "Invalid device name.";
    }

    if (ok && !configurationService_.setWifiSSID(wifiSSID)) {
        logger_.println("Config failed: SSID");
        ok = false;
        errorMessage = "Invalid WiFi SSID.";
    }

    if (ok && changeWifiPassword == "1") {
        if (!configurationService_.setWifiPassword(wifiPassword)) {
            logger_.println("Config failed: WiFi password");
            ok = false;
            errorMessage = "Invalid WiFi password.";
        }
    }

    if (ok && !configurationService_.setMqttServer(mqttServer)) {
        logger_.println("Config failed: MQTT server");
        ok = false;
        errorMessage = "Invalid MQTT server.";
    }

    if (ok && mqttPort != 0 && !configurationService_.setMqttPort(mqttPort)) {
        logger_.println("Config failed: MQTT port");
        ok = false;
        errorMessage = "Invalid MQTT port.";
    }

    // Persist username even if empty (explicitly clear)
    if (ok && !configurationService_.setMqttUsername(mqttUsername)) {
        logger_.println("Config failed: MQTT username");
        ok = false;
        errorMessage = "Invalid MQTT username.";
    }

    // Only update stored password when user explicitly requested it
    if (ok && changeMqttPassword == "1") {
        if (!configurationService_.setMqttPassword(mqttPassword)) {
            logger_.println("Config failed: MQTT password");
            ok = false;
            errorMessage = "Invalid MQTT password.";
        }
    }

    if (ok && !configurationService_.setTimezone(timezone)) {
        logger_.println("Config failed: timezone");
        ok = false;
        errorMessage = "Invalid timezone.";
    }

    if (ok && !configurationService_.setNtpServer1(ntpServer1)) {
        logger_.println("Config failed: NTP server 1");
        ok = false;
        errorMessage = "Invalid NTP server 1.";
    }

    if (ok && !configurationService_.setNtpServer2(ntpServer2)) {
        logger_.println("Config failed: NTP server 2");
        ok = false;
        errorMessage = "Invalid NTP server 2.";
    }

    if (ok && !configurationService_.setPresentationUnit(
            MeasurementType::Temperature, temperatureUnit)) {
        logger_.println("Config failed: Temperature presentation unit");
        ok = false;
        errorMessage = "Could not save Temperature presentation unit.";
    }
    if (ok && !configurationService_.setPresentationUnit(
            MeasurementType::AtmosphericPressure, pressureUnit)) {
        logger_.println("Config failed: Atmospheric Pressure presentation unit");
        ok = false;
        errorMessage = "Could not save Atmospheric Pressure presentation unit.";
    }
    if (ok && !configurationService_.setPresentationUnit(
            MeasurementType::SolarCellTemperature, solarCellTemperatureUnit)) {
        logger_.println("Config failed: Solar Cell Temperature presentation unit");
        ok = false;
        errorMessage = "Could not save Solar Cell Temperature presentation unit.";
    }
    if (ok && !configurationService_.setPresentationUnit(
            MeasurementType::RainDetectorLevel, rainDetectorLevelUnit)) {
        logger_.println("Config failed: Rain Detector Level presentation unit");
        ok = false;
        errorMessage = "Could not save Rain Detector Level presentation unit.";
    }

    if (!ok) {
        logger_.println("Configuration validation failed");
        server_.send(400, "text/html", responsePage("Error", errorMessage.c_str()));
        return;
    }

    logger_.println("Configuration saved");
    server_.send(200, "text/html", responsePage("Success", "Configuration saved. Restarting device..."));
    scheduleRestart();
}

void WebService::handleReset() {
    if (!isProvisioningEnabled()) {
        server_.send(404, "text/plain", "Not Found");
        return;
    }

    if (!configurationService_.resetToDefaults()) {
        logger_.println("Configuration reset failed");
        server_.send(500, "text/html", responsePage("Error", "Reset to defaults failed."));
        return;
    }

    logger_.println("Configuration reset to defaults");
    server_.send(200, "text/html", responsePage("Success", "Configuration reset to defaults. Restarting device..."));
    scheduleRestart();
}

void WebService::handleNotFound() {
    server_.send(404, "text/plain", "Not Found");
}

void WebService::scheduleRestart() {
    if (restartAtMs_ == 0) {
        restartAtMs_ = millis() + 1000;
        logger_.println("Restart scheduled");
    }
}

String WebService::configurationPage() const {
    const Configuration& configuration = configurationService_.getConfiguration();
    String html = "<html><head><title>WeatherStation Setup</title></head><body>";
    html += "<h1>WeatherStation Setup</h1>";
    // Show current network mode: setup AP or connected (with IP)
    if (wifiService_.inSetupAccessPointMode()) {
        html += "<p><strong>Mode:</strong> Setup Access Point</p>";
    } else if (wifiService_.connected()) {
        html += "<p><strong>Mode:</strong> Connected — IP: " + wifiService_.ipAddress() + "</p>";
    }
    html += "<p><em>Device is accessible here while in Setup AP mode or when connected to the configured WiFi. Password fields are never pre-filled for security.</em></p>";
    html += "<form method='post' action='/save' autocomplete='off'>";
    html += "<label>Device name:<br><input type='text' name='deviceName' value='" + configuration.deviceName + "' required></label><br><br>";
    html += "<label>WiFi SSID:<br><input type='text' name='wifiSSID' value='" + configuration.wifiSSID + "' required></label><br><br>";
    html += "<label><input type='checkbox' name='changeWifiPassword' value='1'> Change WiFi password</label>";
    html += "<span style='font-size:small;color:#555;'> Leave unchecked to preserve the current WiFi password.</span><br>";
    html += "<label>WiFi password:<br><input type='password' name='wifiPassword' autocomplete='new-password'></label><br><br>";
    html += "<label>MQTT server:<br><input type='text' name='mqttServer' value='" + configuration.mqttServer + "'></label><br><br>";
    html += "<label>MQTT port:<br><input type='number' name='mqttPort' value='" + String(configuration.mqttPort) + "'></label><br><br>";
    html += "<label><input type='checkbox' name='changeMqttPassword' value='1'> Change MQTT password</label>";
    html += "<span style='font-size:small;color:#555;'> Leave unchecked to preserve the current MQTT password.</span><br>";
    html += "<label>MQTT username:<br><input type='text' name='mqttUsername' value='" + configuration.mqttUsername + "'></label><br><br>";
    html += "<label>MQTT password:<br><input type='password' name='mqttPassword' autocomplete='new-password'></label><br><br>";
    html += "<label>Timezone:<br>" + timezoneSelectHtml(configuration.timezone) + "</label><br><br>";
    html += "<label>NTP server 1:<br><input type='text' name='ntpServer1' value='" + configuration.ntpServer1 + "'></label><br><br>";
    html += "<label>NTP server 2:<br><input type='text' name='ntpServer2' value='" + configuration.ntpServer2 + "'></label><br><br>";
    html += "<h2>Presentation Units</h2>";
    html += "<label>Temperature:<br>";
    html += presentationUnitSelectHtml(
        "temperatureUnit", MeasurementType::Temperature,
        configuration.temperaturePresentationUnit);
    html += "</label><br><br>";
    html += "<label>Atmospheric Pressure:<br>";
    html += presentationUnitSelectHtml(
        "pressureUnit", MeasurementType::AtmosphericPressure,
        configuration.atmosphericPressurePresentationUnit);
    html += "</label><br><br>";
    html += "<label>Solar Cell Temperature:<br>";
    html += presentationUnitSelectHtml(
        "solarCellTemperatureUnit", MeasurementType::SolarCellTemperature,
        configuration.solarCellTemperaturePresentationUnit);
    html += "</label><br><br>";
    html += "<label>Rain Detector Level:<br>";
    html += presentationUnitSelectHtml(
        "rainDetectorLevelUnit", MeasurementType::RainDetectorLevel,
        configuration.rainDetectorLevelPresentationUnit);
    html += "</label><br><br>";
    html += "<button type='submit'>Save</button>";
    html += "</form>";
    html += "<hr>";
    html += "<h2>Reset to defaults</h2>";
    html += "<form method='post' action='/reset' onsubmit='return confirm(\"Reset all configuration and restart the device?\");'>";
    html += "<button type='submit'>Reset to defaults</button>";
    html += "</form>";
    html += "</body></html>";
    return html;
}

String WebService::responsePage(const char* title, const char* message) const {
    String html = "<html><head><title>" + String(title) + "</title></head><body>";
    html += "<h1>" + String(title) + "</h1>";
    html += "<p>" + String(message) + "</p>";
    html += "</body></html>";
    return html;
}

bool WebService::isProvisioningEnabled() const {
    // Allow provisioning page when in setup AP mode or when connected
    // to the configured WiFi network so users can reconfigure remotely.
    return wifiService_.inSetupAccessPointMode() || wifiService_.connected();
}

} // namespace WeatherStation
