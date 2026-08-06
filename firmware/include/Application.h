#pragma once

#include "Logger.h"

namespace WeatherStation {

class Application {
public:
    explicit Application(ILogger& logger);

    void setup();
    void loop();

private:
    ILogger& logger_;
};

} // namespace WeatherStation
