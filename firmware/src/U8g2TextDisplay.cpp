#include "U8g2TextDisplay.h"
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
U8g2TextDisplay::U8g2TextDisplay(I2CBusManager& buses, ILogger& logger)
    : buses_(buses), logger_(logger) {}
void U8g2TextDisplay::transferFailed(const char* reason, unsigned int code) {
    if (!failed_) logger_.warnf("OLED %s failed: %s address=0x%02X %s code=%u",
        phase_, i2cBusName(bus_), static_cast<unsigned>(address_), reason, code);
    failed_ = true;
}
uint8_t U8g2TextDisplay::transfer(u8x8_t* context, uint8_t message, uint8_t count, void* data) {
    auto* self = static_cast<U8g2TextDisplay*>(u8x8_GetUserPtr(context));
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
            self->transferFailed("write buffer", count);
        }
        break;
    case U8X8_MSG_BYTE_END_TRANSFER: {
        const uint8_t status = self->wire_->endTransmission();
        if (status != 0) self->transferFailed("I2C transmission", status);
        break;
    }
    default:
        return 0;
    }
    return self->failed_ ? 0 : 1;
}
bool U8g2TextDisplay::begin(const TextDisplayConfiguration& configuration) {
    end();
    failed_ = false;
    // U8g2 setup routines initialize selected fields, not the entire context.
    // Start from the same zero state as a cold boot before rebinding the driver.
    *display_.getU8g2() = u8g2_t{};
    if (!configuration.enabled || !validateTextDisplayConfiguration(configuration)) return false;
    switch (configuration.type) {
    case TextDisplayType::Ssd1309:
        u8g2_Setup_ssd1309_i2c_128x64_noname2_f(display_.getU8g2(), U8G2_R0, transfer, u8x8_gpio_and_delay_arduino); break;
    case TextDisplayType::Ssd1306:
        u8g2_Setup_ssd1306_i2c_128x64_noname_f(display_.getU8g2(), U8G2_R0, transfer, u8x8_gpio_and_delay_arduino); break;
    case TextDisplayType::Sh1106:
        u8g2_Setup_sh1106_i2c_128x64_noname_f(display_.getU8g2(), U8G2_R0, transfer, u8x8_gpio_and_delay_arduino); break;
    default: return false;
    }
    display_.setUserPtr(this);
    bus_ = configuration.bus;
    wire_ = buses_.wire(bus_);
    if (wire_ == nullptr) {
        logger_.warnf("OLED initialization failed: %s unavailable", i2cBusName(bus_));
        return false;
    }
    address_ = configuration.address;
    BusTimeout timeout(*wire_);
    phase_ = "probe";
    wire_->beginTransmission(address_);
    const uint8_t probeStatus = wire_->endTransmission();
    if (probeStatus != 0) { transferFailed("I2C acknowledgement", probeStatus); return false; }
    phase_ = "initialization";
    display_.setI2CAddress(address_ << 1);
    display_.initDisplay();
    phase_ = "wake";
    display_.setPowerSave(0);
    display_.setFont(u8g2_font_t0_11_tf);
    display_.setFontPosBaseline();
    display_.clearBuffer();
    ready_ = !failed_;
    return ready_;
}
bool U8g2TextDisplay::show(const TextDisplayFrame& frame) {
    if (!ready_ || wire_ == nullptr) return false;
    BusTimeout timeout(*wire_);
    failed_ = false;
    phase_ = "frame";
    display_.clearBuffer();
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        display_.drawUTF8(0, (line + 1) * 10, frame.lines[line]);
    }
    display_.sendBuffer();
    ready_ = !failed_;
    return ready_;
}
void U8g2TextDisplay::end() {
    if (wire_ != nullptr) {
        BusTimeout timeout(*wire_);
        failed_ = false;
        phase_ = "sleep";
        display_.setPowerSave(1);
    }
    ready_ = false;
    wire_ = nullptr;
}
}
