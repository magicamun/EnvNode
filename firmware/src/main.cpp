#include "Application.h"
#include "ConfigurationService.h"
#include "SerialLogger.h"
#include "WiFiService.h"
#include "WebService.h"
#include "MqttService.h"
#include "TimeService.h"

using namespace WeatherStation;

static SerialLogger serialLogger;
static ConfigurationService configurationService;
static WiFiService wifiService(serialLogger, configurationService);
static TimeService timeService(serialLogger, configurationService, wifiService);
static MqttService mqttService(serialLogger, configurationService, wifiService);
static WebService webService(serialLogger, configurationService, wifiService);
static Application app(serialLogger, configurationService, wifiService, webService, mqttService, timeService);

void setup() {
    app.setup();
}

void loop() {
    app.loop();
}
