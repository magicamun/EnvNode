#include "Ssd1309TextDisplay.h"
namespace EnvNode {
namespace {
// Bound a failed transaction without changing the shared bus configuration permanently.
class BusTimeout {
public:
    explicit BusTimeout(TwoWire& wire) : wire_(wire), previous_(wire.getTimeOut()) { wire_.setTimeOut(20); }
    ~BusTimeout() { wire_.setTimeOut(previous_); }
private:
    TwoWire& wire_;
    uint16_t previous_;
};
}
Ssd1309TextDisplay::Ssd1309TextDisplay(I2CBusManager& buses) : buses_(buses) {
    u8g2_Setup_ssd1309_i2c_128x64_noname2_f(display_.getU8g2(), U8G2_R0,
        transfer, u8x8_gpio_and_delay_arduino);
    display_.setUserPtr(this);
}
uint8_t Ssd1309TextDisplay::transfer(u8x8_t* context, uint8_t message, uint8_t count, void* data) {
    auto* self = static_cast<Ssd1309TextDisplay*>(u8x8_GetUserPtr(context));
    if (self == nullptr || self->wire_ == nullptr || self->failed_) return 0;
    switch (message) {
    case U8X8_MSG_BYTE_INIT:
    case U8X8_MSG_BYTE_SET_DC:
        return 1; // I2CBusManager owns begin(), pins and clock.
    case U8X8_MSG_BYTE_START_TRANSFER:
        self->wire_->beginTransmission(self->address_);
        return 1;
    case U8X8_MSG_BYTE_SEND:
        if (self->wire_->write(static_cast<const uint8_t*>(data), count) != count) {
            self->wire_->endTransmission();
            self->failed_ = true;
        }
        break;
    case U8X8_MSG_BYTE_END_TRANSFER:
        if (self->wire_->endTransmission() != 0) self->failed_ = true;
        break;
    default:
        return 0;
    }
    return self->failed_ ? 0 : 1;
}
bool Ssd1309TextDisplay::probe() {
    wire_->beginTransmission(address_);
    return wire_->endTransmission() == 0;
}
bool Ssd1309TextDisplay::begin(const TextDisplayConfiguration& configuration) {
    ready_ = false;
    if (!configuration.enabled || !validateTextDisplayConfiguration(configuration)) return false;
    wire_ = buses_.wire(configuration.bus);
    if (wire_ == nullptr) return false;
    address_ = configuration.address;
    BusTimeout timeout(*wire_);
    if (!probe()) return false;
    failed_ = false;
    display_.setI2CAddress(address_ << 1);
    display_.initDisplay();
    display_.setPowerSave(0);
    display_.setFont(u8g2_font_t0_11_tf);
    display_.setFontPosBaseline();
    ready_ = !failed_;
    return ready_;
}
bool Ssd1309TextDisplay::show(const TextDisplayFrame& frame) {
    if (!ready_ || wire_ == nullptr) return false;
    BusTimeout timeout(*wire_);
    failed_ = false;
    display_.clearBuffer();
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        display_.drawUTF8(0, (line + 1) * 10, frame.lines[line]);
    }
    display_.sendBuffer();
    ready_ = !failed_;
    return ready_;
}
void Ssd1309TextDisplay::end() {
    if (ready_ && wire_ != nullptr) {
        BusTimeout timeout(*wire_);
        failed_ = false;
        display_.setPowerSave(1);
    }
    ready_ = false;
    wire_ = nullptr;
}
}
