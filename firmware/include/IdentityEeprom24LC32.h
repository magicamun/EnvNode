#pragma once

#include "IIdentityStorage.h"
#include "I2CBusManager.h"

namespace EnvNode {

class IdentityEeprom24LC32 : public IIdentityStorage {
public:
    static constexpr size_t Capacity = 4096;
    static constexpr size_t PageSize = 32;

    IdentityEeprom24LC32(I2CBusManager& i2cBusManager, uint8_t deviceAddress);

    bool read(uint16_t address, uint8_t* data, size_t size) override;
    bool write(uint16_t address, const uint8_t* data, size_t size) override;

private:
    bool writePage(uint16_t address, const uint8_t* data, size_t size);
    bool writeAddress(uint16_t address);
    bool waitUntilReady();

    I2CBusManager& i2cBusManager_;
    uint8_t deviceAddress_;
};

} // namespace EnvNode
