#ifndef ENVNODE_SCT_DMA
// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
#include "Sct013Sensor.h"
#ifdef ARDUINO_ARCH_ESP32
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_timer.h>
#include "SctSignal/LowPass.h"
#include "SctSignal/RmsWindow.h"

namespace {
// Stable callback contexts survive slot removal and reconstruction. The mutex
// serializes reconfiguration, timer callbacks and copying a completed window.
struct Channel {
    SemaphoreHandle_t lock=nullptr;
    esp_timer_handle_t timer=nullptr;
    bool running=false;
    adc1_channel_t channel=ADC1_CHANNEL_6;
    esp_adc_cal_characteristics_t calibration{};
    uint32_t hz=0;
    EnvNode::SctTiming timing;
    int64_t windowStart=0;
    int64_t previous=0;
    double first=0;
    RmsWindow full, raw, filtered;
    LowPass filter{200,2000};
    EnvNode::SctWindow completed;
};
constexpr uint8_t pins[]={34,35,36,39};
constexpr adc1_channel_t adcChannels[]={ADC1_CHANNEL_6,ADC1_CHANNEL_7,ADC1_CHANNEL_0,ADC1_CHANNEL_3};
Channel channels[4];
void acquire(void* context) {
    auto& c=*static_cast<Channel*>(context);
    xSemaphoreTake(c.lock,portMAX_DELAY);
    if (c.running) {
        const int64_t now=esp_timer_get_time();
        if (!c.full.count) c.windowStart=now;
        if (c.previous) c.timing.interval(now-c.previous);
        c.previous=now;
        const int code=adc1_get_raw(c.channel);
        const auto mv=esp_adc_cal_raw_to_voltage(code<0?0:code,&c.calibration);
        if (!c.full.count) c.first=mv;
        c.full.add(mv);
        const double ac=mv-c.first;
        const double y=c.hz?c.filter.add(ac):ac;
        if (c.full.count>200) { c.raw.add(ac); c.filtered.add(y); }
        if (c.full.count==2000) {
            auto& w=c.completed;
            ++w.revision;
            w.endMs=static_cast<uint32_t>(now/1000);
            w.bias=c.full.mean; w.minimum=c.full.minimum; w.maximum=c.full.maximum;
            w.timing=c.timing; w.timing.spanUs=now-c.windowStart;
            w.timingErrors=w.timing.shortIntervals+w.timing.longIntervals; w.rawMv=c.raw.acRmsMv(); w.filteredMv=c.filtered.acRmsMv();
            c.full={}; c.raw={}; c.filtered={}; c.timing={};
            c.filter=LowPass(c.hz?c.hz:200,2000);
        }
    }
    xSemaphoreGive(c.lock);
}
}
#endif
namespace EnvNode {
EspSctInput::~EspSctInput() {
#ifdef ARDUINO_ARCH_ESP32
    if(channel_>=0) {
        auto& c=channels[channel_];
        esp_timer_stop(c.timer);
        xSemaphoreTake(c.lock,portMAX_DELAY);
        c.running=false;
        xSemaphoreGive(c.lock);
    }
#endif
}
bool EspSctInput::begin(uint8_t gpio,uint32_t cutoff) {
#ifdef ARDUINO_ARCH_ESP32
    // Runtime rollback may call begin again on the existing live instance.
    if (channel_>=0) return gpio==pins[channel_] && channels[channel_].hz==cutoff;
    int index=0; while(index<4 && pins[index]!=gpio) ++index;
    if(index==4)return false;
    auto& c=channels[index];
    if (!c.lock) c.lock=xSemaphoreCreateMutex();
    if (!c.lock) return false;
    xSemaphoreTake(c.lock,portMAX_DELAY);
    bool ok=!c.running;
    if (ok && !c.timer) {
        esp_timer_create_args_t args{};
        args.callback=acquire; args.arg=&c; args.name="sct013"; args.skip_unhandled_events=true;
        ok=esp_timer_create(&args,&c.timer)==ESP_OK;
    }
    if (ok) {
        c.channel=adcChannels[index];
        ok=adc1_config_width(ADC_WIDTH_BIT_12)==ESP_OK &&
            adc1_config_channel_atten(c.channel,ADC_ATTEN_DB_12)==ESP_OK;
    }
    if (ok) {
        esp_adc_cal_characterize(ADC_UNIT_1,ADC_ATTEN_DB_12,ADC_WIDTH_BIT_12,1100,&c.calibration);
        c.full={}; c.raw={}; c.filtered={}; c.completed={}; c.previous=0; c.timing={};
        c.hz=cutoff; c.filter=LowPass(cutoff?cutoff:200,2000);
        ok=esp_timer_start_periodic(c.timer,500)==ESP_OK;
        c.running=ok;
        if (ok) channel_=index;
    }
    xSemaphoreGive(c.lock);
    return ok;
#else
    (void)gpio; (void)cutoff; return false;
#endif
}
bool EspSctInput::latest(SctWindow& w) {
#ifdef ARDUINO_ARCH_ESP32
    if(channel_>=0) {
        auto& c=channels[channel_];
        if(xSemaphoreTake(c.lock,0)!=pdTRUE) return false;
        w=c.completed; xSemaphoreGive(c.lock);
        return w.revision!=0;
    }
#endif
    return false;
}
}

#endif
