// EnvNode: see repository LICENSE.
#include "I2CBusTypes.h"
namespace EnvNode {
const char* i2cBusName(I2CBus bus) {
    switch (bus) {
        case I2CBus::I2C0: return "I2C0";
        case I2CBus::I2C1: return "I2C1";
        default: return "unknown I2C bus";
    }
}

}
