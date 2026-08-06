#include "WebService.h"
#include <Arduino.h>
#include <WiFi.h>

namespace WeatherStation {

WebService::WebService(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService)
    : logger_(logger)
    , configurationService_(configurationService)
    , wifiService_(wifiService) {
}

void WebService::begin() {
    server_.on("/", HTTP_GET, [this]() { handleRoot(); });
    server_.on("/save", HTTP_POST, [this]() { handleSave(); });
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

    bool ok = true;
    String errorMessage;

    if (!configurationService_.setDeviceName(deviceName)) {
        ok = false;
        errorMessage = "Invalid device name.";
    }

    if (ok && !configurationService_.setWifiSSID(wifiSSID)) {
        ok = false;
        errorMessage = "Invalid WiFi SSID.";
    }

    if (ok && changeWifiPassword == "1") {
        if (!configurationService_.setWifiPassword(wifiPassword)) {
            ok = false;
            errorMessage = "Invalid WiFi password.";
        }
    }

    if (ok && !configurationService_.setMqttServer(mqttServer)) {
        ok = false;
        errorMessage = "Invalid MQTT server.";
    }

    if (ok && mqttPort != 0 && !configurationService_.setMqttPort(mqttPort)) {
        ok = false;
        errorMessage = "Invalid MQTT port.";
    }

    // Persist username even if empty (explicitly clear)
    if (ok && !configurationService_.setMqttUsername(mqttUsername)) {
        ok = false;
        errorMessage = "Invalid MQTT username.";
    }

    // Only update stored password when user explicitly requested it
    if (ok && changeMqttPassword == "1") {
        if (!configurationService_.setMqttPassword(mqttPassword)) {
            ok = false;
            errorMessage = "Invalid MQTT password.";
        }
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
    html += "<label><input type='checkbox' name='changeMqttPassword' value='1'> Change MQTT password</label>";
    html += "<span style='font-size:small;color:#555;'> Leave unchecked to preserve the current MQTT password.</span><br>";
    html += "<label>MQTT username:<br><input type='text' name='mqttUsername' value='" + configuration.mqttUsername + "'></label><br><br>";
    html += "<label>MQTT password:<br><input type='password' name='mqttPassword' autocomplete='new-password'></label><br><br>";
    html += "<button type='submit'>Save</button>";
    html += "</form></body></html>";
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
