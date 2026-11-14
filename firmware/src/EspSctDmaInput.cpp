// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// One SCT slot owns the classic ESP32 ADC DMA / I2S0.
#ifdef ENVNODE_SCT_DMA
#include "Sct013Sensor.h"
#include "SctSignal/LowPass.h"
#include "SctSignal/RmsWindow.h"
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <cstring>
namespace EnvNode {
namespace {
constexpr unsigned Rate=20000, Group=10;
constexpr uint32_t PoolBytes=8192;
EspSctInput* owner=nullptr;
esp_adc_cal_characteristics_t calibration;
uint32_t cutoff, sum, n, lastPoll, lastErrorReport;
constexpr uint32_t RecoveryIntervalMs=5000;
bool initialized=false, running=false, recovering=false, attemptedRecovery=false;
uint32_t lastRecovery=0;
esp_err_t releaseDma() {
    if(running) {
        const auto err=adc_digi_stop();
        if(err!=ESP_OK && err!=ESP_ERR_INVALID_STATE)return err;
        running=false;
    }
    if(initialized) {
        const auto err=adc_digi_deinitialize();
        if(err!=ESP_OK)return err;
        initialized=false;
    }
    return ESP_OK;
}
esp_err_t startDma(int ch) {
    auto err=releaseDma();
    if(err!=ESP_OK)return err;
    adc_digi_init_config_t init{};
    init.max_store_buf_size=PoolBytes;init.conv_num_each_intr=256;init.adc1_chan_mask=1U<<ch;
    err=adc_digi_initialize(&init);
    if(err!=ESP_OK)return err;
    initialized=true;
    adc_digi_pattern_config_t pattern{};
    pattern.atten=ADC_ATTEN_DB_12;pattern.channel=ch;pattern.unit=0;pattern.bit_width=12;
    adc_digi_configuration_t cfg{};
    cfg.conv_limit_en=true;cfg.conv_limit_num=250;
    cfg.sample_freq_hz=Rate;cfg.conv_mode=ADC_CONV_SINGLE_UNIT_1;
    cfg.format=ADC_DIGI_OUTPUT_FORMAT_TYPE1;cfg.pattern_num=1;cfg.adc_pattern=&pattern;
    err=adc_digi_controller_configure(&cfg);
    if(err==ESP_OK)err=adc_digi_start();
    running=err==ESP_OK;
    return err;
}
RmsWindow full,raw,filtered;
LowPass lp(200,2000);
double first;
SctWindow result;
void resetWindow() {
    sum=n=0;full={};raw={};filtered={};lp=LowPass(cutoff?cutoff:200,2000);
}
void invalidate() {
    const uint32_t now=millis();
    if (!result.timingErrors || uint32_t(now-lastErrorReport)>=1000) {
        ++result.revision; result.endMs=lastErrorReport=now;
    }
    result.timingErrors=1;
    resetWindow();
}
void accept(uint16_t code) {
    sum+=code;
    if(++n<Group)return;
    const double mv=esp_adc_cal_raw_to_voltage((sum+Group/2)/Group,&calibration);
    n=sum=0;
    if(!full.count)first=mv;
    full.add(mv);
    const double ac=mv-first, y=cutoff?lp.add(ac):ac;
    if(full.count>200){raw.add(ac);filtered.add(y);}
    if(full.count==2000){
        ++result.revision;result.endMs=millis();result.timingErrors=0;
        result.bias=full.mean;result.minimum=full.minimum;result.maximum=full.maximum;
        result.rawMv=raw.acRmsMv();result.filteredMv=filtered.acRmsMv();resetWindow();
    }
}
}
EspSctInput::~EspSctInput(){
    if(owner==this){releaseDma();owner=nullptr;}
}
bool EspSctInput::begin(uint8_t gpio,uint32_t hz){
    int ch;
    switch(gpio){
        case 34:ch=ADC1_CHANNEL_6;break;
        case 35:ch=ADC1_CHANNEL_7;break;
        case 36:ch=ADC1_CHANNEL_0;break;
        case 39:ch=ADC1_CHANNEL_3;break;
        default:return false;
    }
    if(owner)return owner==this && channel_==ch && cutoff==hz;
    if(startDma(ch)!=ESP_OK){releaseDma();return false;}
    recovering=attemptedRecovery=false;lastRecovery=lastErrorReport=0;
    esp_adc_cal_characterize(ADC_UNIT_1,ADC_ATTEN_DB_12,ADC_WIDTH_BIT_12,1100,&calibration);
    channel_=ch;cutoff=hz;owner=this;result={};resetWindow();lastPoll=millis();return true;
}
bool EspSctInput::latest(SctWindow& w){
    if(owner!=this)return false;
    const uint32_t now=millis();
    if(recovering) {
        invalidate();
        if(!attemptedRecovery || uint32_t(now-lastRecovery)>=RecoveryIntervalMs) {
            attemptedRecovery=true;lastRecovery=now;
            ++result.recoveryAttempts;
            result.lastRecoveryError=startDma(channel_);
            if(result.lastRecoveryError==ESP_OK) {
                ++result.recoverySuccesses;recovering=false;lastPoll=millis();
            }
            // Publish recovery diagnostics, but remain invalid until a full new window.
            ++result.revision;result.endMs=lastErrorReport=millis();
        }
        w=result;return true;
    }
    // Never wait on the main loop. If the consumer fell behind, discard
    // queued frames and the partial window rather than publishing stale data.
    alignas(4) uint8_t buf[512];uint32_t bytes;
    bool discard=uint32_t(now-lastPoll)>=PoolBytes*1000/(Rate*sizeof(adc_digi_output_data_t));
    if (discard) ++result.pollErrors;
    lastPoll=now;
    for(unsigned i=0;i<32;++i){
        bytes=0;
        auto ret=adc_digi_read_bytes(buf,sizeof(buf),&bytes,0);
        if(ret==ESP_ERR_TIMEOUT)break;
        if(ret!=ESP_OK){++result.readErrors;result.lastReadError=ret;discard=true;break;}
        if(bytes%sizeof(adc_digi_output_data_t)){++result.lengthErrors;discard=true;}
        if(!bytes)break;
        if(discard)continue;
        for(unsigned j=0;j<bytes;j+=sizeof(adc_digi_output_data_t)){
            adc_digi_output_data_t v;memcpy(&v,buf+j,sizeof(v));
            if(v.type1.channel!=channel_){++result.channelErrors;result.unexpectedChannel=v.type1.channel;discard=true;break;}
            accept(v.type1.data);
        }
    }
    if(discard){invalidate();recovering=true;}
    w=result;return w.revision!=0;
}
}
#endif
