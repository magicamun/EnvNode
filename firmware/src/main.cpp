#include "Application.h"
#include "DefaultConfigurationService.h"
#include "SerialLogger.h"

using namespace WeatherStation;

static SerialLogger serialLogger;
static DefaultConfigurationService configurationService;
static Application app(serialLogger, configurationService);

void setup() {
    app.setup();
}

void loop() {
    app.loop();
}
