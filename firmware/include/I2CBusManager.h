// EnvNode: see repository LICENSE.
#pragma once
#include "I2CBusController.h"
#include "EnvNodeI2CConfiguration.h"
namespace EnvNode {
// EnvNode adapter: the shared controller does not select active board profiles.
class I2CBusManager : public I2CBusController {
public:
    explicit I2CBusManager(ILogger& logger,
        I2CBusConfiguration identityBus = EnvNodeIdentityI2C);
    bool beginIdentityBus();
    void begin();
    void scanAll();
private:
    const I2CBusConfiguration identityBus_;
};
}
