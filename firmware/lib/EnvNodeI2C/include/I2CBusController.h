// EnvNode: see repository LICENSE.
#pragma once

#include <Wire.h>

#include "II2CBusProvider.h"
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

class I2CBusController : public II2CBusProvider {
public:
    explicit I2CBusController(ILogger& logger);

    bool begin(const I2CBusConfiguration& configuration);
    TwoWire* wire(I2CBus bus) override;
    bool available(I2CBus bus) const;
    void scan(I2CBus bus, I2CScanResult& result);
    const I2CScanResult* lastScan(I2CBus bus) const;

private:
    ILogger& logger_;
    I2CBusConfiguration configurations_[2] = {};
    bool initialized_[2] = {false, false};
    I2CScanResult lastScans_[2];
    bool scanned_[2] = {false, false};
};

} // namespace EnvNode
