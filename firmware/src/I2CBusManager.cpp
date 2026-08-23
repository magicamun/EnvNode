#include "I2CBusManager.h"
#include "BoardProfile.h"

namespace EnvNode {

I2CBusManager::I2CBusManager(ILogger& logger) : logger_(logger) {}

bool I2CBusManager::beginIdentityBus() {
    const size_t busIndex = static_cast<size_t>(I2CBus::I2C0);
    if (initialized_[busIndex]) {
        return true;
    }

    initialized_[busIndex] = Wire.begin(21, 22);
    if (initialized_[busIndex]) {
        logger_.info("Identity bootstrap I2C0 initialized SDA=21 SCL=22");
    } else {
        logger_.error("Identity bootstrap I2C0 initialization failed SDA=21 SCL=22");
    }
    return initialized_[busIndex];
}

void I2CBusManager::begin() {
    const BoardProfile& board = currentBoardProfile();
    for (size_t index = 0; index < board.i2cBusCount; ++index) {
        const BoardI2CBusCapability* capability = &board.i2cBuses[index];
        const size_t busIndex = static_cast<size_t>(capability->bus);
        if (busIndex >= 2 || initialized_[busIndex]) continue;
        TwoWire* instance = capability->bus == I2CBus::I2C0 ? &Wire : &Wire1;
        initialized_[busIndex] = instance->begin(capability->sda.number, capability->scl.number);
        if (initialized_[busIndex]) {
            logger_.infof("%s initialized SDA=%u SCL=%u", i2cBusName(capability->bus),
                capability->sda.number, capability->scl.number);
        } else {
            logger_.errorf("%s initialization failed SDA=%u SCL=%u",
                i2cBusName(capability->bus),
                capability->sda.number, capability->scl.number);
        }
    }
}

TwoWire* I2CBusManager::wire(I2CBus bus) {
    if (!available(bus)) return nullptr;
    return bus == I2CBus::I2C0 ? &Wire : &Wire1;
}

bool I2CBusManager::available(I2CBus bus) const {
    const size_t index = static_cast<size_t>(bus);
    return index < 2 && initialized_[index];
}

void I2CBusManager::scan(I2CBus bus, I2CScanResult& result) {
    result = I2CScanResult(bus);
    TwoWire* instance = wire(bus);
    if (instance == nullptr) {
        logger_.warnf("%s scan unavailable: bus is not initialized", i2cBusName(bus));
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
