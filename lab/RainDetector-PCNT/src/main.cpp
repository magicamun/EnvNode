#include <Arduino.h>
#include <driver/pcnt.h>
#include <esp_timer.h>
#include <soc/pcnt_struct.h>

#include "LabConfig.h"

namespace {
constexpr pcnt_unit_t CounterUnit = PCNT_UNIT_0;
constexpr uint32_t CounterMask = 1U << CounterUnit;
constexpr int16_t CounterLimit = 32767;

void check(esp_err_t result, const char* operation) {
    if (result == ESP_OK) {
        return;
    }
    Serial.printf("# error,%s,%s\n", operation, esp_err_to_name(result));
    Serial.flush();
    // Do not publish plausible-looking measurements after a driver error.
    while (true) {
        delay(1000);
    }
}
}  // namespace

void setup() {
    Serial.begin(LabConfig::SerialBaud);
    pcnt_config_t config = {};
    config.pulse_gpio_num = LabConfig::RainFreqGpio;
    config.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    config.unit = CounterUnit;
    config.channel = PCNT_CHANNEL_0;
    config.pos_mode = PCNT_COUNT_INC;
    config.neg_mode = PCNT_COUNT_DIS;
    config.lctrl_mode = PCNT_MODE_KEEP;
    config.hctrl_mode = PCNT_MODE_KEEP;
    config.counter_h_lim = CounterLimit;
    config.counter_l_lim = -1;
    check(pcnt_unit_config(&config), "configure");
    check(pcnt_counter_pause(CounterUnit), "pause");
    // The actively driven 3.3 V oscillator needs no internal pull resistors.
    check(gpio_set_pull_mode(static_cast<gpio_num_t>(LabConfig::RainFreqGpio),
                             GPIO_FLOATING), "pull_mode");
    check(pcnt_filter_disable(CounterUnit), "filter_disable");
    check(pcnt_intr_disable(CounterUnit), "interrupt_disable");
    check(pcnt_event_enable(CounterUnit, PCNT_EVT_H_LIM), "limit_event");
    Serial.println("timestamp_ms,pulse_count,frequency_hz");
    Serial.flush();
}

void loop() {
    check(pcnt_counter_clear(CounterUnit), "clear");
    // Only the high-limit event is enabled. Its raw interrupt bit latches
    // even with CPU interrupts disabled, detecting ANY wrap in this window.
    // These two register accesses are specific to the classic ESP32.
    PCNT.int_clr.val = CounterMask;
    const int64_t startUs = esp_timer_get_time();
    check(pcnt_counter_resume(CounterUnit), "resume");
    while (esp_timer_get_time() - startUs < LabConfig::MeasurementWindowUs) {
        delay(1);  // Yield; PCNT counts autonomously. No GPIO polling.
    }
    check(pcnt_counter_pause(CounterUnit), "pause");
    const int64_t endUs = esp_timer_get_time();
    int16_t pulseCount = 0;
    check(pcnt_get_counter_value(CounterUnit, &pulseCount), "read");
    const bool overflow = (PCNT.int_raw.val & CounterMask) != 0;
    if (overflow) {
        // The hardware resets at 32767; the residual count is not a total.
        Serial.printf("%llu,%d,nan\n",
                      static_cast<unsigned long long>(endUs / 1000), pulseCount);
    } else {
        const double frequencyHz = pulseCount * 1000000.0 / (endUs - startUs);
        Serial.printf("%llu,%d,%.3f\n",
                      static_cast<unsigned long long>(endUs / 1000),
                      pulseCount, frequencyHz);
    }
    Serial.flush();  // Complete output outside the next measurement window.
}
