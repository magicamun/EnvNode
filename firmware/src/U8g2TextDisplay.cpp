#include "U8g2TextDisplay.h"
#include <new>
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
bool U8g2TextDisplay::sendOledPacket(const uint8_t* bytes, size_t length) {
    if (!wire_ || failed_) return false;
    wire_->beginTransmission(address_);
    const size_t written = wire_->write(bytes, length);
    // Always finish the transaction, including short writes.
    const uint8_t status = wire_->endTransmission();
    if (written != length) transferFailed("short packet", static_cast<unsigned>(written));
    else if (status != 0) transferFailed("I2C transmission", status);
    return !failed_;
}
uint8_t U8g2TextDisplay::commands(u8x8_t* context, uint8_t message, uint8_t count, void* data) {
    auto* self = static_cast<U8g2TextDisplay*>(u8x8_GetUserPtr(context));
    if (!self || !self->wire_ || self->failed_) return 0;
    bool ok = true;
    switch (message) {
    case U8X8_MSG_CAD_INIT: return 1;
    case U8X8_MSG_CAD_START_TRANSFER: ok = self->commands_.flush(); break;
    case U8X8_MSG_CAD_SEND_CMD: ok = self->commands_.command(count); break;
    case U8X8_MSG_CAD_SEND_ARG: ok = self->commands_.argument(count); break;
    case U8X8_MSG_CAD_SEND_DATA: ok = self->commands_.data(static_cast<const uint8_t*>(data), count); break;
    case U8X8_MSG_CAD_END_TRANSFER: ok = self->commands_.flush(); break;
    default: return 0;
    }
    if (!ok && !self->failed_) self->transferFailed("command framing", message);
    return ok ? 1 : 0;
}
bool U8g2TextDisplay::begin(const TextDisplayConfiguration& configuration) {
    end();
    failed_ = false;
    commands_.reset();
    // U8g2 setup routines initialize selected fields, not the entire context.
    // Start from the same zero state as a cold boot before rebinding the driver.
    *display_.getU8g2() = u8g2_t{};
    if (!configuration.enabled || !validateTextDisplayConfiguration(configuration)) return false;
    if (!framebuffer_) framebuffer_.reset(new (std::nothrow) uint8_t[128 * 64 / 8]);
    if (!framebuffer_) { logger_.warn("OLED framebuffer allocation failed"); return false; }
    switch (configuration.type) {
    case TextDisplayType::Ssd1309:
        u8g2_SetupDisplay(display_.getU8g2(), u8x8_d_ssd1309_128x64_noname2, commands, u8x8_byte_empty, u8x8_gpio_and_delay_arduino); break;
    case TextDisplayType::Ssd1306:
        u8g2_SetupDisplay(display_.getU8g2(), u8x8_d_ssd1306_128x64_noname, commands, u8x8_byte_empty, u8x8_gpio_and_delay_arduino); break;
    case TextDisplayType::Sh1106:
        u8g2_SetupDisplay(display_.getU8g2(), u8x8_d_sh1106_128x64_noname, commands, u8x8_byte_empty, u8x8_gpio_and_delay_arduino); break;
    default: return false;
    }
    // Use per-instance storage; the stock full-buffer setup shares a static buffer.
    u8g2_SetupBuffer(display_.getU8g2(), framebuffer_.get(), 8, u8g2_ll_hvline_vertical_top_lsb, U8G2_R0);
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
    if (configuration.type == TextDisplayType::Ssd1309) {
        // SSD1309 datasheet: unlock command access even if the controller is
        // still powered in a locked state; STOP does not reset its registers.
        commands_.command(0xFD); commands_.argument(0x12); commands_.flush();
    }
    display_.initDisplay();
    if (configuration.type == TextDisplayType::Ssd1309) {
        // U8g2 NONAME2 leaves multiplex ratio at its power-on default.
        commands_.command(0xA8); commands_.argument(0x3F);
        commands_.command(0xD3); commands_.argument(0x00); commands_.flush();
    }
    if (failed_) return false;
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
    commands_.reset();
    phase_ = "frame";
    display_.clearBuffer();
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        display_.drawUTF8(0, (line + 1) * 10, frame.lines[line]);
    }
    display_.sendBuffer();
    if (!failed_) {
        // Wake only after the complete RAM image was accepted.
        phase_ = "wake";
        display_.setPowerSave(0);
    }
    ready_ = !failed_;
    return ready_;
}
void U8g2TextDisplay::end() {
    if (wire_ != nullptr) {
        BusTimeout timeout(*wire_);
        failed_ = false;
        commands_.reset();
        phase_ = "sleep";
        display_.setPowerSave(1);
    }
    ready_ = false;
    wire_ = nullptr;
}
}
