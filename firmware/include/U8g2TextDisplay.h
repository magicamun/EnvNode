#pragma once
#include <U8g2lib.h>
#include <memory>
#include "ITextDisplay.h"
#include "I2CBusManager.h"
namespace EnvNode {
class U8g2TextDisplay : public ITextDisplay {
public:
    U8g2TextDisplay(I2CBusManager& buses, ILogger& logger);
    bool begin(const TextDisplayConfiguration& configuration) override;
    bool show(const TextDisplayFrame& frame) override;
    void end() override;
private:
    static uint8_t transfer(u8x8_t* context, uint8_t message, uint8_t count, void* data);
    void transferFailed(const char* reason, unsigned int code);
    I2CBusManager& buses_;
    ILogger& logger_;
    I2CBus bus_ = I2CBus::I2C0;
    const char* phase_ = "idle";
    TwoWire* wire_ = nullptr;
    U8G2 display_;
    std::unique_ptr<uint8_t[]> framebuffer_;
    uint8_t address_ = 0x3C;
    bool failed_ = false;
    bool ready_ = false;
};
}
