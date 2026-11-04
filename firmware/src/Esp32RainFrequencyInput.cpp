// EnvNode-owned: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include "Esp32RainFrequencyInput.h"
#include "RainFrequencyConversion.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <sdkconfig.h>
#endif

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32)
#include <driver/pcnt.h>
#include <esp_timer.h>
#include <soc/pcnt_struct.h>
namespace {
// Main-loop-owned resource pool; each live input owns a distinct PCNT unit.
uint32_t claimedUnits = 0;
constexpr uint64_t WindowUs = 250000;
}
#endif
namespace EnvNode {
Esp32RainFrequencyInput::~Esp32RainFrequencyInput() { stop(); }
void Esp32RainFrequencyInput::stop() {
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32)
    if (unit_ >= 0) {
        const auto unit = static_cast<pcnt_unit_t>(unit_);
        pcnt_counter_pause(unit);
        pcnt_intr_disable(unit);
        pcnt_event_disable(unit, PCNT_EVT_H_LIM);
        pcnt_set_pin(unit, PCNT_CHANNEL_0, PCNT_PIN_NOT_USED, PCNT_PIN_NOT_USED);
        PCNT.int_clr.val = 1U << unit_;
        claimedUnits &= ~(1U << unit_);
    }
#endif
    unit_ = -1;
    running_ = false;
    gateOpen_ = false;
}
bool Esp32RainFrequencyInput::begin(uint8_t gpio) {
    stop();
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32)
    if (!GPIO_IS_VALID_GPIO(gpio)) return false;
    for (int candidate = 0; candidate < PCNT_UNIT_MAX; ++candidate) {
        if (!(claimedUnits & (1U << candidate))) {
            unit_ = candidate;
            claimedUnits |= 1U << candidate;
            break;
        }
    }
    if (unit_ < 0) return false;
    const auto unit = static_cast<pcnt_unit_t>(unit_);
    pcnt_config_t config = {};
    config.pulse_gpio_num = gpio;
    config.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    config.unit = unit;
    config.channel = PCNT_CHANNEL_0;
    config.pos_mode = PCNT_COUNT_INC;
    config.neg_mode = PCNT_COUNT_DIS;
    config.lctrl_mode = PCNT_MODE_KEEP;
    config.hctrl_mode = PCNT_MODE_KEEP;
    config.counter_h_lim = 32767;
    config.counter_l_lim = -1;
    if (pcnt_unit_config(&config) != ESP_OK
        || pcnt_counter_pause(unit) != ESP_OK
        || gpio_set_pull_mode(static_cast<gpio_num_t>(gpio), GPIO_FLOATING) != ESP_OK
        || pcnt_filter_disable(unit) != ESP_OK
        || pcnt_intr_disable(unit) != ESP_OK
        || pcnt_event_enable(unit, PCNT_EVT_H_LIM) != ESP_OK
        || !startWindow()) {
        stop();
        return false;
    }
    running_ = true;
    gateOpen_ = true;
    return true;
#else
    (void)gpio;
    return false; // Unsupported hardware never pretends to acquire data.
#endif
}
bool Esp32RainFrequencyInput::startWindow() {
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32)
    const auto unit = static_cast<pcnt_unit_t>(unit_);
    if (pcnt_counter_clear(unit) != ESP_OK) return false;
    PCNT.int_clr.val = 1U << unit_;
    startUs_ = static_cast<uint64_t>(esp_timer_get_time());
    return pcnt_counter_resume(unit) == ESP_OK;
#else
    return false;
#endif
}
RainFrequencyPoll Esp32RainFrequencyInput::poll(RainFrequencyWindow& result) {
    result = RainFrequencyWindow{};
    if (!running_) return RainFrequencyPoll::Failed;
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32)
    // Restart only on the NEXT service pass, after observers/controllers have
    // handled the completed gate. Automatic heater transitions then happen
    // between gates instead of contaminating the next frequency window.
    if (!gateOpen_) {
        if (!startWindow()) { stop(); return RainFrequencyPoll::Failed; }
        gateOpen_ = true;
        return RainFrequencyPoll::Pending;
    }
    const uint64_t nowUs = static_cast<uint64_t>(esp_timer_get_time());
    if (nowUs - startUs_ < WindowUs) return RainFrequencyPoll::Pending;
    const auto unit = static_cast<pcnt_unit_t>(unit_);
    if (pcnt_counter_pause(unit) != ESP_OK) { stop(); return RainFrequencyPoll::Failed; }
    const uint64_t endUs = static_cast<uint64_t>(esp_timer_get_time());
    int16_t count = 0;
    if (pcnt_get_counter_value(unit, &count) != ESP_OK || count < 0) {
        stop(); return RainFrequencyPoll::Failed;
    }
    const bool overflow = (PCNT.int_raw.val & (1U << unit_)) != 0;
    result = rainFrequencyWindow(startUs_, endUs, static_cast<uint32_t>(count), overflow);
    gateOpen_ = false;
    return RainFrequencyPoll::Window;
#else
    return RainFrequencyPoll::Failed;
#endif
}
} // namespace EnvNode
