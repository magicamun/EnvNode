// EnvNode: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <Arduino.h>
#include <driver/pcnt.h>
#include <esp_timer.h>
#include <soc/pcnt_struct.h>

#include "LabConfig.h"
#include "HeaterControl.h"
#include "LabRainAutomation.h"
#include <RainDetectorEvaluation.h>

namespace {
constexpr pcnt_unit_t CounterUnit = PCNT_UNIT_0;
constexpr uint32_t CounterMask = 1U << CounterUnit;
constexpr int16_t CounterLimit = 32767;

RainLab::HeaterControl heater;
RainLab::LabRainAutomation automation;
uint64_t lastAutoReportMs = 0;
bool pwmReady = false;
EnvNode::RainDetectorEvaluation rainEvaluation;
static_assert(sizeof(EnvNode::RainDetectorEvaluation) <= 256, "Evaluation RAM budget");
uint64_t lastEvaluationReportMs = 0;
EnvNode::RainEstimate lastEstimate = EnvNode::RainEstimate::Unknown;

void evaluateRain(uint64_t timeMs, float frequencyHz, unsigned heaterPercent) {
    const auto evaluation = rainEvaluation.update(timeMs, frequencyHz, heaterPercent);
    if (evaluation.newWetting || evaluation.estimate != lastEstimate
        || timeMs - lastEvaluationReportMs >= 1000) {
        Serial.printf("# evaluation,%llu,%.3f,%.3f,%.3f,%u,%s,%u\n",
            static_cast<unsigned long long>(timeMs), evaluation.fastHz, evaluation.slowHz,
            evaluation.trendHzPerSecond, evaluation.valid ? 1 : 0,
            EnvNode::rainEstimateName(evaluation.estimate), evaluation.newWetting ? 1 : 0);
        lastEvaluationReportMs = timeMs;
        lastEstimate = evaluation.estimate;
    }
}

void stopHeater() {
    if (pwmReady) ledcWrite(LabConfig::HeaterPwmChannel, 0);
    else digitalWrite(LabConfig::HeaterGpio, LOW);
    heater.off();
}

void reportHeater(const char* reason) {
    Serial.printf("# heater,%llu,%u,%lld,%s\n",
        static_cast<unsigned long long>(esp_timer_get_time() / 1000),
        heater.percent(),
        static_cast<long long>(heater.continuous() ? -1
            : heater.remainingUs(esp_timer_get_time()) / 1000),
        reason);
}

void writeHeaterPwm();

void reportInterval(const char* reason, int64_t plannedUs, int64_t appliedUs) {
    Serial.printf("# interval,%s,%lld,%lld,%llu,%u,%u,%lld,%s\n", reason,
        static_cast<long long>(plannedUs), static_cast<long long>(appliedUs),
        static_cast<unsigned long long>(heater.phase()), heater.percent(), heater.step(),
        static_cast<long long>(heater.intervalUs()), heater.ascending() ? "up" : "down");
}

void serviceTimeout() {
    const int64_t plannedUs = heater.deadlineUs();
    if (heater.interval() && esp_timer_get_time() - plannedUs >= heater.intervalUs()) {
        stopHeater();
        reportHeater("interval_overrun");
        Serial.println("# error,interval,missed full interval; sequence stopped");
        return;
    }
    if (heater.advanceInterval(esp_timer_get_time())) {
        writeHeaterPwm();
        reportInterval("step", plannedUs, esp_timer_get_time());
        reportHeater("interval");
    }
    if (heater.expire(esp_timer_get_time())) {
        stopHeater();
        reportHeater("timeout");
    }
}

void writeHeaterPwm() {
    const unsigned maximumDuty = (1U << LabConfig::HeaterPwmResolutionBits) - 1;
    ledcWrite(LabConfig::HeaterPwmChannel,
        (heater.percent() * maximumDuty + 50) / 100);
}

void reportAutoConfig() {
    const auto& c = automation.config();
    Serial.printf("# auto_config,edge_ms=%u,drop_hz=%u,drop_permille=%u,end_s=%u,dry_hz=%u,hysteresis_hz=%u,dry_s=%u\n",
        c.edgeMs, c.dropHz, c.dropPermille, c.endSeconds, c.dryHz, c.hysteresisHz, c.drySeconds);
}

void reportAuto(uint64_t timeMs, const char* reason) {
    const auto& r = automation.result();
    Serial.printf("# auto,%llu,%s,%u,%u,%s,%u,%u,%u,%llu,%llu\n",
        static_cast<unsigned long long>(timeMs), reason, automation.enabled(), r.valid,
        RainLab::autoRainName(r.rain), r.wetting, r.heating, r.dry,
        static_cast<unsigned long long>(r.quietMs), static_cast<unsigned long long>(r.dryMs));
    lastAutoReportMs = timeMs;
}

void disableAuto(const char* reason) {
    if (!automation.enabled()) return;
    automation.disable();
    reportAuto(esp_timer_get_time() / 1000, reason);
}

void evaluateAuto(uint64_t timeMs, float frequencyHz, unsigned measuredPercent) {
    if (!automation.enabled()) return;
    const auto before = automation.result();
    const auto current = automation.update(timeMs, frequencyHz, measuredPercent);
    if (current.heating != before.heating) {
        if (current.heating) { heater.startContinuous(100); writeHeaterPwm(); }
        else stopHeater();
        reportHeater(current.heating ? "auto_wetting"
            : current.valid && current.dry ? "auto_dry" : "auto_signal_invalid");
    }
    if (current.wetting || current.rain != before.rain || current.valid != before.valid
        || current.heating != before.heating || current.dry != before.dry
        || timeMs - lastAutoReportMs >= 1000) {
        const char* reason = !current.valid && before.heating && !current.heating
            ? "signal_invalid" : current.wetting ? "wetting" : current.dry && !before.dry ? "dry"
            : current.rain == RainLab::AutoRain::EndEstimated && before.rain != current.rain
                ? "rain_end" : !current.valid ? "guard_or_invalid" : "sample";
        reportAuto(timeMs, reason);
    }
}

void handleCommand(const char* line) {
    if (RainLab::autoWord(line, "ON")) {
        stopHeater();
        automation.enable();
        reportHeater("auto_start_off");
        reportAutoConfig();
        reportAuto(esp_timer_get_time() / 1000, "enabled");
        return;
    }
    if (RainLab::autoWord(line, "OFF")) {
        disableAuto("disabled");
        stopHeater(); reportHeater("off");
        return;
    }
    if (RainLab::autoWord(line, "STATUS")) {
        reportAutoConfig(); reportAuto(esp_timer_get_time() / 1000, "status");
        return;
    }
    RainLab::AutoConfig config;
    if (RainLab::parseAutoConfig(line, config)) {
        if (!automation.configure(config)) Serial.println("# error,auto,disable AUTO before configuring");
        else reportAutoConfig();
        return;
    }

    const auto command = RainLab::parseCommand(line);
    switch (command.kind) {
        case RainLab::CommandKind::Interval: {
            disableAuto("manual_interval");
            const int64_t startedUs = esp_timer_get_time();
            heater.startInterval(command.percent, command.seconds, startedUs);
            writeHeaterPwm();
            reportInterval("start", startedUs, esp_timer_get_time());
            reportHeater("interval_start");
            break;
        }
        case RainLab::CommandKind::Heat:
            disableAuto("manual_heat");
            heater.start(command.percent, command.seconds, esp_timer_get_time());
            writeHeaterPwm(); reportHeater("command"); break;
        case RainLab::CommandKind::HeatContinuous:
            disableAuto("manual_heat");
            heater.startContinuous(command.percent);
            writeHeaterPwm(); reportHeater("command"); break;
        case RainLab::CommandKind::Resume:
            if (!heater.resume()) {
                Serial.println("# error,command,no previous heater level; use HEAT percent ON");
                break;
            }
            disableAuto("manual_resume");
            writeHeaterPwm(); reportHeater("command"); break;
        case RainLab::CommandKind::Off:
            disableAuto("off");
            stopHeater(); reportHeater("off"); break;
        case RainLab::CommandKind::Status:
            reportAutoConfig(); reportAuto(esp_timer_get_time() / 1000, "status");
            reportHeater("status");
            if (heater.interval()) reportInterval("status", heater.deadlineUs() - heater.intervalUs(), esp_timer_get_time());
            break;
        case RainLab::CommandKind::Help:
            Serial.printf("# commands: AUTO ON | AUTO OFF | AUTO STATUS | AUTO CONFIG <edge_ms> <drop_hz> <drop_permille> <end_s> <dry_hz> <hysteresis_hz> <dry_s> | INTERVALL <step dividing 100> <1..300 seconds> | HEAT <1..100 percent> <1..%u seconds> | HEAT <percent> ON | HEAT ON | HEAT OFF | OFF | STATUS | HELP\n",
                LabConfig::HeaterMaxDurationSeconds);
            break;
        default:
            Serial.println("# error,command,invalid command; type HELP");
    }
}

void serviceCommands() {
    static char line[64];
    static size_t length = 0;
    static bool discard = false;
    // Bounded work: even a continuous input stream cannot starve the timeout.
    for (unsigned count = 0; count < sizeof(line) && Serial.available(); ++count) {
        const char character = static_cast<char>(Serial.read());
        if (character == '\r' || character == '\n') {
            if (discard) Serial.println("# error,command,line too long or invalid character");
            else if (length) { line[length] = '\0'; handleCommand(line); }
            length = 0;
            discard = false;
        } else if (!discard) {
            if ((character < 32 && character != '\t') || character > 126
                || length == sizeof(line) - 1) discard = true;
            else line[length++] = character;
        }
    }
}

void check(esp_err_t result, const char* operation) {
    if (result == ESP_OK) {
        return;
    }
    automation.disable();
    stopHeater();
    Serial.printf("# error,%s,%s\n", operation, esp_err_to_name(result));
    Serial.flush();
    // Do not publish plausible-looking measurements after a driver error.
    while (true) {
        delay(1000);
    }
}
}  // namespace

void setup() {
    // Drive the MOSFET input LOW before initializing Serial or PCNT.
    digitalWrite(LabConfig::HeaterGpio, LOW);
    pinMode(LabConfig::HeaterGpio, OUTPUT);
    Serial.begin(LabConfig::SerialBaud);
    if (ledcSetup(LabConfig::HeaterPwmChannel, LabConfig::HeaterPwmFrequencyHz,
                  LabConfig::HeaterPwmResolutionBits) == 0) {
        check(ESP_FAIL, "heater_pwm_setup");
    }
    ledcWrite(LabConfig::HeaterPwmChannel, 0);
    ledcAttachPin(LabConfig::HeaterGpio, LabConfig::HeaterPwmChannel);
    pwmReady = true;
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
    Serial.println("timestamp_ms,pulse_count,frequency_hz,heater_percent");
    reportHeater("boot_off");
    reportAutoConfig();
    reportAuto(esp_timer_get_time() / 1000, "boot_disabled");
    Serial.printf("# evaluation_core_bytes,%u\n", static_cast<unsigned>(sizeof(rainEvaluation)));
    Serial.flush();
}

void loop() {
    // Changes occur between windows, so heater_percent applies to the full window.
    serviceTimeout();
    serviceCommands();
    serviceTimeout();
    const unsigned measuredHeaterPercent = heater.percent();
    check(pcnt_counter_clear(CounterUnit), "clear");
    // Only the high-limit event is enabled. Its raw interrupt bit latches
    // even with CPU interrupts disabled, detecting ANY wrap in this window.
    // These two register accesses are specific to the classic ESP32.
    PCNT.int_clr.val = CounterMask;
    const int64_t startUs = esp_timer_get_time();
    check(pcnt_counter_resume(CounterUnit), "resume");
    // Shorten the final window so switching is not delayed by a full window.
    const int64_t windowEndUs = heater.interval()
        && heater.deadlineUs() < startUs + LabConfig::MeasurementWindowUs
        ? heater.deadlineUs() : startUs + LabConfig::MeasurementWindowUs;
    while (esp_timer_get_time() < windowEndUs) {
        delay(1);  // Yield; PCNT counts autonomously. No GPIO polling.
    }
    check(pcnt_counter_pause(CounterUnit), "pause");
    const int64_t endUs = esp_timer_get_time();
    serviceTimeout();
    Serial.printf("# window,%lld,%lld,%u\n", static_cast<long long>(startUs),
        static_cast<long long>(endUs), measuredHeaterPercent);
    int16_t pulseCount = 0;
    check(pcnt_get_counter_value(CounterUnit, &pulseCount), "read");
    const bool overflow = (PCNT.int_raw.val & CounterMask) != 0;
    if (overflow) {
        // The hardware resets at 32767; the residual count is not a total.
        Serial.printf("%llu,%d,nan,%u\n",
                      static_cast<unsigned long long>(endUs / 1000), pulseCount, measuredHeaterPercent);
    } else {
        const double frequencyHz = pulseCount * 1000000.0 / (endUs - startUs);
        Serial.printf("%llu,%d,%.3f,%u\n",
                      static_cast<unsigned long long>(endUs / 1000),
                      pulseCount, frequencyHz, measuredHeaterPercent);
    }
    evaluateRain(static_cast<uint64_t>(endUs / 1000),
        overflow ? NAN : static_cast<float>(pulseCount * 1000000.0 / (endUs - startUs)),
        measuredHeaterPercent);
    evaluateAuto(static_cast<uint64_t>(endUs / 1000),
        overflow || endUs - startUs < 100000 ? NAN
            : static_cast<float>(pulseCount * 1000000.0 / (endUs - startUs)),
        measuredHeaterPercent);
    serviceTimeout();
    Serial.flush();  // Complete output outside the next measurement window.
}
