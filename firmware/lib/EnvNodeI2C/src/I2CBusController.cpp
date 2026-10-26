// EnvNode: see repository LICENSE.
#include "I2CBusController.h"
namespace EnvNode {
I2CBusController::I2CBusController(ILogger& logger) : logger_(logger) {}
bool I2CBusController::begin(const I2CBusConfiguration& configuration) {
    const size_t index = static_cast<size_t>(configuration.bus);
    if (index >= 2) return false;
    if (initialized_[index]) {
        return configurations_[index].sda == configuration.sda
            && configurations_[index].scl == configuration.scl;
    }
    TwoWire& instance = configuration.bus == I2CBus::I2C0 ? Wire : Wire1;
    initialized_[index] = instance.begin(configuration.sda, configuration.scl);
    if (initialized_[index]) {
        configurations_[index] = configuration;
        logger_.infof("%s initialized SDA=%u SCL=%u", i2cBusName(configuration.bus),
            configuration.sda, configuration.scl);
    } else {
        logger_.errorf("%s initialization failed SDA=%u SCL=%u", i2cBusName(configuration.bus),
            configuration.sda, configuration.scl);
    }
    return initialized_[index];
}
TwoWire* I2CBusController::wire(I2CBus bus) {
    if (!available(bus)) return nullptr;
    return bus == I2CBus::I2C0 ? &Wire : &Wire1;
}

bool I2CBusController::available(I2CBus bus) const {
    const size_t index = static_cast<size_t>(bus);
    return index < 2 && initialized_[index];
}

const I2CScanResult* I2CBusController::lastScan(I2CBus bus) const {
    const size_t index = static_cast<size_t>(bus);
    return index < 2 && scanned_[index] ? &lastScans_[index] : nullptr;
}

void I2CBusController::scan(I2CBus bus, I2CScanResult& result) {
    result = I2CScanResult(bus);
    const size_t busIndex = static_cast<size_t>(bus);
    const auto saveResult = [this, busIndex, &result]() {
        if (busIndex < 2) {
            lastScans_[busIndex] = result;
            scanned_[busIndex] = true;
        }
    };
    TwoWire* instance = wire(bus);
    if (instance == nullptr) {
        logger_.warnf("%s scan unavailable: bus is not initialized", i2cBusName(bus));
        saveResult();
        return;
    }

    for (uint8_t address = 0x08; address <= 0x77; ++address) {
        instance->beginTransmission(address);
        const uint8_t probeResult = instance->endTransmission();
        if (probeResult == 0) {
            result.addresses[result.addressCount++] = address;
        } else if (probeResult != 2 && probeResult != 3) {
            ++result.probeErrorCount;
        }
    }

    result.status = result.probeErrorCount == 0
        ? I2CScanStatus::Complete
        : I2CScanStatus::CompleteWithProbeErrors;
    saveResult();
    if (result.probeErrorCount == 0) {
        logger_.infof("%s scan complete: %u devices", i2cBusName(bus),
            static_cast<unsigned int>(result.addressCount));
    } else {
        logger_.warnf("%s scan complete: %u devices, %u probe errors",
            i2cBusName(bus),
            static_cast<unsigned int>(result.addressCount),
            static_cast<unsigned int>(result.probeErrorCount));
    }
}

} // namespace EnvNode
