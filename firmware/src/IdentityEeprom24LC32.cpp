#include "IdentityEeprom24LC32.h"

#include <Arduino.h>

namespace EnvNode {
namespace {

const I2CBus IdentityBus = I2CBus::I2C0;
const size_t ReadChunkSize = 32;
const uint32_t WriteTimeoutMs = 10;

} // namespace

IdentityEeprom24LC32::IdentityEeprom24LC32(
    I2CBusManager& i2cBusManager,
    uint8_t deviceAddress)
    : i2cBusManager_(i2cBusManager)
    , deviceAddress_(deviceAddress) {
}

bool IdentityEeprom24LC32::read(
    uint16_t address,
    uint8_t* data,
    size_t size) {
    if (data == nullptr || size == 0 || address >= Capacity
        || size > Capacity - static_cast<size_t>(address)) {
        return false;
    }
    TwoWire* wire = i2cBusManager_.wire(IdentityBus);
    if (wire == nullptr) return false;

    size_t offset = 0;
    while (offset < size) {
        const size_t remaining = size - offset;
        const size_t chunkSize = remaining < ReadChunkSize
            ? remaining
            : ReadChunkSize;
        const uint16_t currentAddress =
            static_cast<uint16_t>(address + offset);

        wire->beginTransmission(deviceAddress_);
        if (!writeAddress(currentAddress)
            || wire->endTransmission(false) != 0) {
            return false;
        }

        const size_t received = wire->requestFrom(
            deviceAddress_, static_cast<uint8_t>(chunkSize));
        if (received != chunkSize) return false;
        for (size_t index = 0; index < chunkSize; ++index) {
            if (!wire->available()) return false;
            data[offset + index] = static_cast<uint8_t>(wire->read());
        }
        offset += chunkSize;
    }
    return true;
}

bool IdentityEeprom24LC32::write(
    uint16_t address,
    const uint8_t* data,
    size_t size) {
    if (data == nullptr || size == 0 || address >= Capacity
        || size > Capacity - static_cast<size_t>(address)) {
        return false;
    }

    size_t offset = 0;
    while (offset < size) {
        const uint16_t currentAddress = static_cast<uint16_t>(address) + offset;
        const size_t pageRemaining = PageSize - (currentAddress % PageSize);
        const size_t chunkSize = (size - offset < pageRemaining)
            ? size - offset
            : pageRemaining;
        if (!writePage(currentAddress, data + offset, chunkSize)) return false;
        offset += chunkSize;
    }
    return true;
}

bool IdentityEeprom24LC32::writePage(
    uint16_t address,
    const uint8_t* data,
    size_t size) {
    TwoWire* wire = i2cBusManager_.wire(IdentityBus);
    if (wire == nullptr || size == 0 || size > PageSize) return false;

    wire->beginTransmission(deviceAddress_);
    if (!writeAddress(address) || wire->write(data, size) != size
        || wire->endTransmission() != 0) return false;
    return waitUntilReady();
}

bool IdentityEeprom24LC32::writeAddress(uint16_t address) {
    TwoWire* wire = i2cBusManager_.wire(IdentityBus);
    return wire != nullptr
        && wire->write(static_cast<uint8_t>(address >> 8)) == 1
        && wire->write(static_cast<uint8_t>(address & 0xFF)) == 1;
}

bool IdentityEeprom24LC32::waitUntilReady() {
    TwoWire* wire = i2cBusManager_.wire(IdentityBus);
    if (wire == nullptr) return false;

    const uint32_t startedAt = millis();
    do {
        wire->beginTransmission(deviceAddress_);
        if (wire->endTransmission() == 0) return true;
        delay(1);
    } while (static_cast<uint32_t>(millis() - startedAt) < WriteTimeoutMs);
    return false;
}

} // namespace EnvNode
