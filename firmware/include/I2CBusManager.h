#pragma once

#include <Wire.h>

#include "HardwareResources.h"
#include "Logger.h"

namespace EnvNode {

constexpr size_t MaximumI2CScanAddressCount = 0x77 - 0x08 + 1;

enum class I2CScanStatus : uint8_t {
    Complete,
    CompleteWithProbeErrors,
    BusUnavailable,
};

struct I2CScanResult {
    explicit I2CScanResult(I2CBus busId = I2CBus::I2C0)
        : bus(busId) {
    }

    I2CBus bus;
    I2CScanStatus status = I2CScanStatus::BusUnavailable;
    uint8_t addresses[MaximumI2CScanAddressCount] = {};
    size_t addressCount = 0;
    size_t probeErrorCount = 0;
};

class I2CBusManager {
public:
    explicit I2CBusManager(ILogger& logger);

    bool beginIdentityBus();
    void begin();
    TwoWire* wire(I2CBus bus);
    bool available(I2CBus bus) const;
    void scan(I2CBus bus, I2CScanResult& result);

private:
    ILogger& logger_;
    bool initialized_[2] = {false, false};
};

} // namespace EnvNode
