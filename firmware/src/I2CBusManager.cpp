#include "I2CBusManager.h"

namespace EnvNode {

I2CBusManager::I2CBusManager(ILogger& logger) : logger_(logger) {}

void I2CBusManager::begin() {
    const BoardCapabilities& board = BoardCapabilities::current();
    for (size_t index = 0; index < board.i2cBusCount(); ++index) {
        const BoardI2CBusCapability* capability = board.i2cBusAt(index);
        if (capability == nullptr) continue;
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

} // namespace EnvNode
