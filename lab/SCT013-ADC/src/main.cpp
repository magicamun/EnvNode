// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
#include <Arduino.h>
#include <Preferences.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include "RmsWindow.h"
#include "RawDiagnostics.h"
#include "LowPass.h"
#include "CurrentGate.h"

namespace {
constexpr uint32_t samplePeriodUs = 500; // 2 ksample/s
constexpr uint32_t samplesPerWindow = 2000; // 1 s, 50 nominal mains cycles
constexpr double dividerGain = 0.5; // R1 = R4 = 10k
constexpr double ratedAmps = 5.0;
Preferences preferences;
esp_adc_cal_characteristics_t adcCalibration;
float ratedVolts = 0.333f;
float correction = 1.0f;
uint16_t filterHz = 0; // Session-only experimental diagnostic; default off.
CurrentGate gate;
bool gateEnabled = false;
bool diagnostics = false;
bool paused = false;
uint32_t dumpCount = 0;
uint16_t rawSamples[samplesPerWindow];
uint16_t mvSamples[samplesPerWindow];
uint32_t sampleTimes[samplesPerWindow];
RawDiagnostics<samplesPerWindow> rawDiagnostics;

class AnalogSampler {
public:
    virtual uint16_t readRaw() = 0;
    virtual ~AnalogSampler() = default;
};
class GpioSampler : public AnalogSampler {
public:
    uint8_t pin = 35;
    adc1_channel_t channel() const { return pin == 35 ? ADC1_CHANNEL_7 : ADC1_CHANNEL_6; }
    bool select(uint8_t selected) {
        if (selected != 34 && selected != 35) return false;
        const auto next = selected == 35 ? ADC1_CHANNEL_7 : ADC1_CHANNEL_6;
        if (adc1_config_channel_atten(next, ADC_ATTEN_DB_12) != ESP_OK) return false;
        pin = selected;
        return true;
    }
    uint16_t readRaw() override {
        return adc1_get_raw(channel());
    }
};
GpioSampler hardware;

void settings() {
    Serial.printf("# gate=%s; off_mA=%d; on_mA=%d\n", gateEnabled ? "on" : "off", gate.offMa, gate.onMa);
    Serial.printf("# filter_hz=%u; compare_skip=200; filter setting is session-only\n", filterHz);
    Serial.printf("# Mini port=%s; GPIO%u; sensor=5A/%.3fV; divider=0.5; calibration=%.5f; A/V=%.5f\n",
                  hardware.pin == 35 ? "ADC2" : "ADC1", hardware.pin, ratedVolts, correction, ratedAmps / (ratedVolts * dividerGain) * correction);
}
void help() {
    Serial.println("# Commands (Enter): sensor 333 | sensor 1000 | cal <factor> | port adc1 | port adc2 | pin 34 | pin 35 | status | help");
    Serial.println("# Gate: gate <off_mA> <on_mA> (example: gate 60 80) | gate off; enable filter first.");
    Serial.println("# Filter: filter off | filter 100 | filter 200 | filter 300 | filter 500");
    Serial.println("# FILTER compares raw/filtered RMS over the SAME last 1800 samples; original CSV and dumps stay raw.");
    Serial.println("# Diagnostics: diag on | diag off | dump | dump full | pause | resume");
    Serial.println("# Dump captures the next window, prints 200 or all 2000 samples, then pauses.");
    Serial.println("# Pin, sensor and calibration saved in NVS. Sensor change resets calibration to 1.");
    Serial.println("# No noise subtraction. No real-power measurement. Verify selected sensor label!");
    Serial.println("# ms,offset_mV,ac_rms_mV,current_A,min_mV,max_mV,samples,late_samples,flags");
}
void command(const String &s) {
    if (s == "sensor 333" || s == "sensor 1000") {
        ratedVolts = s == "sensor 333" ? 0.333f : 1.0f;
        correction = 1;
        gateEnabled = false; gate.active = false;
        preferences.putFloat("ratedV", ratedVolts);
        preferences.putFloat("correction", correction);
        settings();
    } else if (s == "pin 34" || s == "pin 35" || s == "port adc1" || s == "port adc2") {
        if (hardware.select((s == "pin 35" || s == "port adc2") ? 35 : 34)) {
            preferences.putUChar("adcPin", hardware.pin);
        gateEnabled = false; gate.active = false;
            settings();
        } else Serial.println("# ADC pin configuration failed; selection unchanged.");
    } else if (s.startsWith("cal ")) {
        char *end = nullptr;
        const char *arg = s.c_str() + 4;
        const float value = strtof(arg, &end);
        if (end == arg || *end || !isfinite(value) || value < 0.1f || value > 10) {
            Serial.println("# Invalid calibration; use factor 0.1..10, decimal point.");
        } else {
            correction = value;
        gateEnabled = false; gate.active = false;
            preferences.putFloat("correction", correction);
            settings();
        }
    } else if (s == "gate off") {
        gateEnabled = false;
        gate.active = false;
        settings();
    } else if (s.startsWith("gate ")) {
        const char *arg = s.c_str() + 5;
        char *end;
        long low = strtol(arg, &end, 10);
        if (end == arg || *end != ' ') { Serial.println("# Use gate <off_mA> <on_mA>."); return; }
        const char *second = end + 1;
        long high = strtol(second, &end, 10);
        if (end == second || *end || low < 1 || high <= low || high > 5000) {
            Serial.println("# Require 1 <= off_mA < on_mA <= 5000."); return;
        }
        if (!filterHz) { Serial.println("# Enable filter first."); return; }
        gate.offMa = low; gate.onMa = high; gate.active = false; gateEnabled = true;
        settings();
    } else if (s.startsWith("filter ")) {
        if (s == "filter off") filterHz = 0;
        else if (s == "filter 100") filterHz = 100;
        else if (s == "filter 200") filterHz = 200;
        else if (s == "filter 300") filterHz = 300;
        else if (s == "filter 500") filterHz = 500;
        else { Serial.println("# Invalid filter: off, 100, 200, 300 or 500 Hz."); return; }
        gateEnabled = false; gate.active = false;
        settings();
    } else if (s == "diag on" || s == "diag off") {
        diagnostics = s == "diag on";
        Serial.printf("# Diagnostics %s\n", diagnostics ? "on" : "off");
    } else if (s == "dump" || s == "dump full") {
        dumpCount = s == "dump" ? 200 : samplesPerWindow;
        paused = false;
        Serial.println("# Capture queued; output follows acquisition.");
    } else if (s == "pause" || s == "resume") {
        paused = s == "pause";
        dumpCount = 0;
        Serial.printf("# Measurement %s\n", paused ? "paused" : "running");
    } else if (s == "status") {
        settings();
        Serial.printf("# diagnostics=%s; measurement=%s\n", diagnostics ? "on" : "off", paused ? "paused" : "running");
    }
    else if (s == "help") help();
    else if (s.length()) Serial.println("# Unknown command; use help.");
}
void readCommands() {
    static String line;
    static bool overflow = false;
    while (Serial.available()) {
        const char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (!overflow) { line.trim(); command(line); }
            else Serial.println("# Command too long.");
            line = ""; overflow = false;
        } else if (line.length() < 64 && !overflow) line += c;
        else overflow = true;
    }
}
void measure(AnalogSampler &sampler) {
    RmsWindow window;
    uint32_t late = 0;
    uint32_t next = micros();
    const uint32_t started = next;
    for (uint32_t i = 0; i < samplesPerWindow; ++i) {
        while (static_cast<int32_t>(micros() - next) < 0) delayMicroseconds(10);
        if (static_cast<int32_t>(micros() - next) > static_cast<int32_t>(samplePeriodUs)) {
            ++late;
            next = micros(); // no burst of catch-up samples
        }
        sampleTimes[i] = micros() - started;
        rawSamples[i] = sampler.readRaw();
        mvSamples[i] = esp_adc_cal_raw_to_voltage(rawSamples[i], &adcCalibration);
        window.add(mvSamples[i]);
        next += samplePeriodUs;
    }
    const double amps = window.acRmsMv() / 1000 * ratedAmps / (ratedVolts * dividerGain) * correction;
    String flags;
    if (window.mean < 1400 || window.mean > 1900) flags += "BIAS_CHECK|";
    if (window.minimum < 150 || window.maximum > 2450) flags += "ADC_RANGE|";
    if (window.minimum < 50 || window.maximum > 3100) flags += "POSSIBLE_CLIP|";
    if (late) flags += "TIMING|";
    if (flags.isEmpty()) flags = "OK";
    Serial.printf("%lu,%.1f,%.3f,%.5f,%.0f,%.0f,%lu,%lu,%s\n",
                  static_cast<unsigned long>(millis()), window.mean, window.acRmsMv(), amps,
                  window.minimum, window.maximum, static_cast<unsigned long>(window.count),
                  static_cast<unsigned long>(late), flags.c_str());
    if (filterHz) {
        // Post-process captured values: no extra work or serial output during sampling.
        LowPass filter(filterHz, 1000000.0 / samplePeriodUs);
        RmsWindow rawComparison, filteredComparison;
        for (uint32_t i = 0; i < samplesPerWindow; ++i) {
            const double centered = mvSamples[i] - window.mean;
            const double filtered = filter.add(centered);
            if (i >= 200) {
                rawComparison.add(centered);
                filteredComparison.add(filtered);
            }
        }
        const double scale = ratedAmps / (ratedVolts * dividerGain) * correction / 1000;
        Serial.printf("# FILTER hz=%u,raw_rms_mV=%.3f,filtered_rms_mV=%.3f,raw_A=%.5f,filtered_A=%.5f,samples=%lu,skip=200,flags=%s\n",
                      filterHz, rawComparison.acRmsMv(), filteredComparison.acRmsMv(),
                      rawComparison.acRmsMv()*scale, filteredComparison.acRmsMv()*scale,
                      (unsigned long)filteredComparison.count, flags.c_str());
        if (gateEnabled) {
            if (flags == "OK") {
                const double output = gate.apply(filteredComparison.acRmsMv() * scale);
                Serial.printf("# GATE output_A=%.5f,state=%s\n", output, gate.active ? "on" : "zero");
            } else {
                Serial.printf("# GATE output_A=NA,flags=%s\n", flags.c_str());
            }
        }
    }
    if (diagnostics || dumpCount) {
        rawDiagnostics.calculate(rawSamples, mvSamples, window.mean);
        const auto &r = rawDiagnostics;
        Serial.printf("# RAW mean=%.2f,rms_counts=%.3f,min=%.0f,max=%.0f,p01=%u,p50=%u,p99=%u,rail0=%lu,rail4095=%lu\n",
                      r.raw.mean, r.raw.acRmsMv(), r.raw.minimum, r.raw.maximum,
                      r.p01, r.p50, r.p99, (unsigned long)r.lowRail, (unsigned long)r.highRail);
        Serial.printf("# NOISE beyond20mV=%lu,beyond50mV=%lu,span_us=%lu\n",
                      (unsigned long)r.beyond20, (unsigned long)r.beyond50,
                      (unsigned long)(sampleTimes[samplesPerWindow - 1] - sampleTimes[0]));
    }
    if (dumpCount) {
        settings();
        Serial.println("# SAMPLE,index,t_us,raw,mV");
        for (uint32_t i = 0; i < dumpCount; ++i) {
            Serial.printf("SAMPLE,%lu,%lu,%u,%u\n", (unsigned long)i,
                          (unsigned long)sampleTimes[i], rawSamples[i], mvSamples[i]);
        }
        dumpCount = 0;
        paused = true;
        Serial.println("# Dump complete; paused. Use resume or dump.");
    }
}
}
void setup() {
    Serial.begin(115200);
    delay(500);
    if (!preferences.begin("sct013-mini", false)) Serial.println("# NVS unavailable; settings may not persist.");
    ratedVolts = preferences.getFloat("ratedV", 0.333f);
    if (fabsf(ratedVolts - 0.333f) > 0.0001f && fabsf(ratedVolts - 1.0f) > 0.0001f) ratedVolts = 0.333f;
    correction = preferences.getFloat("correction", 1.0f);
    if (!isfinite(correction) || correction < 0.1f || correction > 10) correction = 1;
        gateEnabled = false; gate.active = false;
    adc1_config_width(ADC_WIDTH_BIT_12);
    uint8_t savedPin = preferences.getUChar("adcPin", 35);
    if (savedPin != 34 && savedPin != 35) savedPin = 35;
    if (!hardware.select(savedPin)) {
        Serial.println("# ADC configuration failed; measurements paused.");
        paused = true;
    }
    const auto source = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12,
                                                ADC_WIDTH_BIT_12, 1100, &adcCalibration);
    Serial.println("\n# SCT013Head Lab - EnvNode Mini - default port ADC2 / GPIO35 - WiFi disabled");
    Serial.printf("# ADC calibration source: %s\n", source == ESP_ADC_CAL_VAL_EFUSE_TP ? "eFuse two-point" :
                  source == ESP_ADC_CAL_VAL_EFUSE_VREF ? "eFuse Vref" : "default 1100mV (approximate)");
    Serial.println("# Head: R1-R4=10k, C1=10nF, C2=10uF; no sensor: current is noise, not load current.");
    settings(); help();
}
void loop() {
    readCommands();
    if (!paused) measure(hardware);
    delay(5);
}
