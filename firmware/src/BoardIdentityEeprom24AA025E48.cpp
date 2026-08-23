#include "BoardIdentityEeprom24AA025E48.h"
#include "BoardIdentityCodec.h"

#include <Arduino.h>

namespace EnvNode {
namespace {

const I2CBus IdentityBus = I2CBus::I2C0;
const uint8_t DeviceAddress = 0x50;
const size_t PageSize = 16;
const size_t BoardIdentityStorageSize = BoardIdentityCodec::EncodedSize;
const uint32_t WriteTimeoutMs = 10;

} // namespace

BoardIdentityEeprom24AA025E48::BoardIdentityEeprom24AA025E48(
    I2CBusManager& i2cBusManager)
    : i2cBusManager_(i2cBusManager) {
}

bool BoardIdentityEeprom24AA025E48::read(
    uint8_t address,
    uint8_t* data,
    size_t size) {
    if (data == nullptr || size == 0
        || static_cast<size_t>(address) + size > BoardIdentityStorageSize) {
        return false;
    }

    TwoWire* wire = i2cBusManager_.wire(IdentityBus);
    if (wire == nullptr) {
        return false;
    }

    wire->beginTransmission(DeviceAddress);
    wire->write(address);
    if (wire->endTransmission(false) != 0) {
        return false;
    }

    const size_t received = wire->requestFrom(
        static_cast<uint8_t>(DeviceAddress),
        static_cast<uint8_t>(size));
    if (received != size) {
        return false;
    }
    for (size_t index = 0; index < size; ++index) {
        if (!wire->available()) {
            return false;
        }
        data[index] = static_cast<uint8_t>(wire->read());
    }
    return true;
}

bool BoardIdentityEeprom24AA025E48::write(
    uint8_t address,
    const uint8_t* data,
    size_t size) {
    if (data == nullptr || size == 0
        || static_cast<size_t>(address) + size > BoardIdentityStorageSize) {
        return false;
    }

    size_t offset = 0;
    while (offset < size) {
        const size_t pageRemaining = PageSize - ((address + offset) % PageSize);
        const size_t chunkSize = (size - offset < pageRemaining)
            ? size - offset
            : pageRemaining;
        if (!writePage(
                static_cast<uint8_t>(address + offset),
                data + offset,
                chunkSize)) {
            return false;
        }
        offset += chunkSize;
    }
    return true;
}

bool BoardIdentityEeprom24AA025E48::writePage(
    uint8_t address,
    const uint8_t* data,
    size_t size) {
    TwoWire* wire = i2cBusManager_.wire(IdentityBus);
    if (wire == nullptr || size == 0 || size > PageSize) {
        return false;
    }

    wire->beginTransmission(DeviceAddress);
    if (wire->write(address) != 1 || wire->write(data, size) != size
        || wire->endTransmission() != 0) {
        return false;
    }
    return waitUntilReady();
}

bool BoardIdentityEeprom24AA025E48::waitUntilReady() {
    TwoWire* wire = i2cBusManager_.wire(IdentityBus);
    if (wire == nullptr) {
        return false;
    }

    const uint32_t startedAt = millis();
    do {
        wire->beginTransmission(DeviceAddress);
        if (wire->endTransmission() == 0) {
            return true;
        }
        delay(1);
    } while (static_cast<uint32_t>(millis() - startedAt) < WriteTimeoutMs);
    return false;
}

} // namespace EnvNode
