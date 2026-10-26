// EnvNode: see repository LICENSE.
#include "DeviceRestartExecutor.h"
#include <Arduino.h>
namespace EnvNode {
namespace {
class PlatformRestartDriver : public IDeviceRestartDriver {
public:
    void restart() override {
#if defined(ARDUINO_ARCH_ESP32)
        ESP.restart();
#endif
    }
};
IDeviceRestartDriver& platformRestartDriver() {
    static PlatformRestartDriver driver;
    return driver;
}
}
DeviceRestartExecutor::DeviceRestartExecutor(ILogger& logger, IDeviceRestartDriver* driver)
    : logger_(logger), driver_(driver == nullptr ? platformRestartDriver() : *driver) {}
void DeviceRestartExecutor::execute() {
    logger_.info("Device restart executed");
    driver_.restart();
}
}
