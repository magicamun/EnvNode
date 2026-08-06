#include "Application.h"
#include "ConfigurationService.h"
#include "SerialLogger.h"

using namespace WeatherStation;

static SerialLogger serialLogger;
static ConfigurationService configurationService;
static Application app(serialLogger, configurationService);

void setup() {
    app.setup();
}

void loop() {
    app.loop();
}
