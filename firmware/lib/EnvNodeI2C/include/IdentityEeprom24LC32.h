#pragma once

#include "IIdentityStorage.h"
#include "II2CBusProvider.h"

namespace EnvNode {

class IdentityEeprom24LC32 : public IIdentityStorage {
public:
    static constexpr size_t Capacity = 4096;
    static constexpr size_t PageSize = 32;

    IdentityEeprom24LC32(II2CBusProvider& i2cBusManager, uint8_t deviceAddress,
        I2CBus bus = I2CBus::I2C0);

    bool read(uint16_t address, uint8_t* data, size_t size) override;
    bool write(uint16_t address, const uint8_t* data, size_t size) override;

private:
    bool writePage(uint16_t address, const uint8_t* data, size_t size);
    bool writeAddress(uint16_t address);
    bool waitUntilReady();

    II2CBusProvider& i2cBusManager_;
    uint8_t deviceAddress_;
    const I2CBus bus_;
};

} // namespace EnvNode
