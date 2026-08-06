#include "Application.h"
#include "ConfigurationService.h"
#include "SerialLogger.h"
#include "WiFiService.h"
#include "WebService.h"
#include "MqttService.h"

using namespace WeatherStation;

static SerialLogger serialLogger;
static ConfigurationService configurationService;
static WiFiService wifiService(serialLogger, configurationService);
static MqttService mqttService(serialLogger, configurationService, wifiService);
static WebService webService(serialLogger, configurationService, wifiService);
static Application app(serialLogger, configurationService, wifiService, webService, mqttService);

void setup() {
    app.setup();
}

void loop() {
    app.loop();
}
