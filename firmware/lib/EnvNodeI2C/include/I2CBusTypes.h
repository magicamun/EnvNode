// EnvNode: see repository LICENSE.
#pragma once
#include <cstdint>
namespace EnvNode {
enum class I2CBus : uint8_t { I2C0 = 0, I2C1 = 1 };
struct I2CBusConfiguration {
    I2CBus bus;
    uint8_t sda;
    uint8_t scl;
};
const char* i2cBusName(I2CBus bus);
} // namespace EnvNode
