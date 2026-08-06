#include "Application.h"
#include "ConfigurationService.h"
#include "SerialLogger.h"
#include "WiFiService.h"

using namespace WeatherStation;

static SerialLogger serialLogger;
static ConfigurationService configurationService;
static WiFiService wifiService(serialLogger, configurationService);
static Application app(serialLogger, configurationService, wifiService);

void setup() {
    app.setup();
}

void loop() {
    app.loop();
}
