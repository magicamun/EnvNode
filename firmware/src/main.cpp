#include "Application.h"
#include "SerialLogger.h"

using namespace WeatherStation;

static SerialLogger serialLogger;
static Application app(serialLogger);

void setup() {
    app.setup();
}

void loop() {
    app.loop();
}
