#include "Application.h"
#include "ConfigurationService.h"
#include "SerialLogger.h"
#include "WiFiService.h"
#include "WebService.h"

using namespace WeatherStation;

static SerialLogger serialLogger;
static ConfigurationService configurationService;
static WiFiService wifiService(serialLogger, configurationService);
static WebService webService(serialLogger, configurationService, wifiService);
static Application app(serialLogger, configurationService, wifiService, webService);

void setup() {
    app.setup();
}

void loop() {
    app.loop();
}
