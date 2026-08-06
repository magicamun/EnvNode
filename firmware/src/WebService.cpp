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

    if (ok && !configurationService_.setWifiPassword(wifiPassword)) {
        ok = false;
        errorMessage = "Invalid WiFi password.";
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
    html += "<form method='post' action='/save'>";
    html += "<label>Device name:<br><input type='text' name='deviceName' value='" + configuration.deviceName + "' required></label><br><br>";
    html += "<label>WiFi SSID:<br><input type='text' name='wifiSSID' value='" + configuration.wifiSSID + "' required></label><br><br>";
    html += "<label>WiFi password:<br><input type='password' name='wifiPassword'></label><br><br>";
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
    return wifiService_.inSetupAccessPointMode();
}

} // namespace WeatherStation
