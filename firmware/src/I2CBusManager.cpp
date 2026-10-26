// EnvNode: see repository LICENSE.
#include "I2CBusManager.h"
#include "BoardProfile.h"
namespace EnvNode {
I2CBusManager::I2CBusManager(ILogger& logger, I2CBusConfiguration identityBus)
    : I2CBusController(logger), identityBus_(identityBus) {}
bool I2CBusManager::beginIdentityBus() {
    return I2CBusController::begin(identityBus_);
}
void I2CBusManager::begin() {
    const BoardProfile& board = currentBoardProfile();
    for (size_t index = 0; index < board.i2cBusCount; ++index) {
        const BoardI2CBusCapability& bus = board.i2cBuses[index];
        I2CBusController::begin({bus.bus, bus.sda.number, bus.scl.number});
    }
}
void I2CBusManager::scanAll() {
    const BoardProfile& board = currentBoardProfile();
    for (size_t index = 0; index < board.i2cBusCount; ++index) {
        I2CScanResult result;
        scan(board.i2cBuses[index].bus, result);
    }
}
}
