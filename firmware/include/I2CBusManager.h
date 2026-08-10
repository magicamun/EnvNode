#pragma once

#include <Wire.h>

#include "HardwareResources.h"
#include "Logger.h"

namespace WeatherStation {

class I2CBusManager {
public:
    explicit I2CBusManager(ILogger& logger);

    void begin();
    TwoWire* wire(I2CBus bus);
    bool available(I2CBus bus) const;

private:
    ILogger& logger_;
    bool initialized_[2] = {false, false};
};

} // namespace WeatherStation
