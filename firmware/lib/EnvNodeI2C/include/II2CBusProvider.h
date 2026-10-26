// EnvNode: see repository LICENSE.
#pragma once
#include <Wire.h>
#include "I2CBusTypes.h"
namespace EnvNode {
class II2CBusProvider {
public:
    virtual ~II2CBusProvider() = default;
    virtual TwoWire* wire(I2CBus bus) = 0;
};
}
