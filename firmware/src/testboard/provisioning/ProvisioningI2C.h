// EnvNode: see repository LICENSE.
#pragma once
#include <Wire.h>
#include "II2CBusProvider.h"
#include "TestBoardProvisioning.h"
namespace EnvNode {
class ProvisioningI2C : public IProvisioningConnection, public II2CBusProvider {
public:
    ProvisioningI2C(uint8_t sda, uint8_t scl) : sda_(sda), scl_(scl) {}
    bool begin() override {
        active_ = Wire.begin(sda_, scl_);
        if (active_) Wire.setTimeOut(50);
        return active_;
    }
    void end() override {
        Wire.end();
        // Driver deletion alone does not restore the GPIO configuration.
        pinMode(sda_, INPUT); pinMode(scl_, INPUT);
        active_ = false;
    }
    TwoWire* wire(I2CBus bus) override { return active_ && bus == I2CBus::I2C0 ? &Wire : nullptr; }
private:
    uint8_t sda_, scl_;
    bool active_ = false;
};
}
