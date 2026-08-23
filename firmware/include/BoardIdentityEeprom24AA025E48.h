#pragma once

#include "IBoardIdentityStorage.h"
#include "I2CBusManager.h"

namespace EnvNode {

class BoardIdentityEeprom24AA025E48 : public IBoardIdentityStorage {
public:
    explicit BoardIdentityEeprom24AA025E48(I2CBusManager& i2cBusManager);

    bool read(uint8_t address, uint8_t* data, size_t size) override;
    bool write(uint8_t address, const uint8_t* data, size_t size) override;

private:
    bool writePage(uint8_t address, const uint8_t* data, size_t size);
    bool waitUntilReady();

    I2CBusManager& i2cBusManager_;
};

} // namespace EnvNode
