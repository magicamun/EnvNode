#pragma once
#include <U8g2lib.h>
#include "ITextDisplay.h"
#include "I2CBusManager.h"
namespace EnvNode {
class Ssd1309TextDisplay : public ITextDisplay {
public:
    explicit Ssd1309TextDisplay(I2CBusManager& buses);
    bool begin(const TextDisplayConfiguration& configuration) override;
    bool show(const TextDisplayFrame& frame) override;
    void end() override;
private:
    static uint8_t transfer(u8x8_t* context, uint8_t message, uint8_t count, void* data);
    bool probe();
    I2CBusManager& buses_;
    TwoWire* wire_ = nullptr;
    U8G2 display_;
    uint8_t address_ = 0x3C;
    bool failed_ = false;
    bool ready_ = false;
};
}
