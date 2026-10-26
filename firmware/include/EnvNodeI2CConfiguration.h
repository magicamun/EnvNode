// EnvNode: see repository LICENSE.
#pragma once
#include "I2CBusTypes.h"
namespace EnvNode {
// Available before the EEPROM identifies the active EnvNode board profile.
constexpr I2CBusConfiguration EnvNodeIdentityI2C = {I2CBus::I2C0, 21, 22};
}
