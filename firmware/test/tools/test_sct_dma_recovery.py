# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
# Host fault injection for the production DMA implementation with mocked ESP APIs.
from pathlib import Path
r=Path(__file__).resolve().parents[3]
s=(r/'firmware/src/EspSctDmaInput.cpp').read_text()
s='\n'.join(x for x in s.splitlines() if not x.startswith('#include'))
h='''#include <cstdint>
#include <cstring>
#include <cassert>
#include <cmath>
#include <algorithm>
#define ENVNODE_SCT_DMA
using esp_err_t=int;
constexpr int ESP_OK=0,ESP_ERR_TIMEOUT=1,ESP_ERR_INVALID_STATE=259;
constexpr int ADC1_CHANNEL_6=6,ADC1_CHANNEL_7=7,ADC1_CHANNEL_0=0,ADC1_CHANNEL_3=3,ADC_ATTEN_DB_12=3,ADC_CONV_SINGLE_UNIT_1=0,ADC_DIGI_OUTPUT_FORMAT_TYPE1=0,ADC_UNIT_1=0,ADC_WIDTH_BIT_12=12;
uint32_t tick=0; uint32_t millis(){return tick;}
int reads=0,starts=0,stops=0,deinits=0,queued=0,readError=0,startError=0;
struct adc_digi_init_config_t{unsigned max_store_buf_size,conv_num_each_intr,adc1_chan_mask;};
struct adc_digi_pattern_config_t{unsigned atten,channel,unit,bit_width;};
struct adc_digi_configuration_t{bool conv_limit_en;unsigned conv_limit_num,sample_freq_hz,conv_mode,format,pattern_num;adc_digi_pattern_config_t* adc_pattern;};
struct adc_digi_output_data_t{struct {uint16_t data:12;uint16_t channel:4;} type1;};
struct esp_adc_cal_characteristics_t{};
int adc_digi_initialize(adc_digi_init_config_t*){return 0;}
int adc_digi_controller_configure(adc_digi_configuration_t*){return 0;}
int adc_digi_start(){++starts;queued=0;return startError;}
int adc_digi_stop(){++stops;return 0;}
int adc_digi_deinitialize(){++deinits;return 0;}
void esp_adc_cal_characterize(int,int,int,int,esp_adc_cal_characteristics_t*){}
unsigned esp_adc_cal_raw_to_voltage(unsigned x,esp_adc_cal_characteristics_t*){return x;}
int adc_digi_read_bytes(uint8_t* b,uint32_t len,uint32_t* n,int){++reads;*n=0;if(readError)return readError;if(!queued)return ESP_ERR_TIMEOUT;unsigned count=std::min(unsigned(queued),len/2);auto p=reinterpret_cast<adc_digi_output_data_t*>(b);for(unsigned i=0;i<count;++i){p[i].type1.channel=7;p[i].type1.data=1600;}queued-=count;*n=count*2;return 0;}
namespace EnvNode {
struct SctWindow{bool pausedForUpload=false;unsigned readErrors=0,channelErrors=0,pollErrors=0,lengthErrors=0,recoveryAttempts=0,recoverySuccesses=0,revision=0,endMs=0,timingErrors=0;int lastReadError=0,unexpectedChannel=-1,lastRecoveryError=0;float bias=0,minimum=0,maximum=0,rawMv=0,filteredMv=0;};
class EspSctInput{int channel_=-1;public:static bool pauseForUpload();static void resumeAfterUploadFailure();~EspSctInput();bool begin(uint8_t,uint32_t);bool latest(SctWindow&);};
}
'''
h+='\n#include "'+str(r/'firmware/include/SctSignal/LowPass.h')+'"\n#include "'+str(r/'firmware/include/SctSignal/RmsWindow.h')+'"\n'
t='''
int main(){using namespace EnvNode;EspSctInput input;SctWindow w;assert(input.begin(35,200));
readError=259;input.latest(w);assert(w.timingErrors && w.readErrors==1);int oldReads=reads;
startError=259;input.latest(w);assert(w.recoveryAttempts==1 && w.recoverySuccesses==0);
for(int i=0;i<100;++i){tick+=10;input.latest(w);}assert(starts==2 && reads==oldReads);
tick=5000;startError=0;readError=0;input.latest(w);assert(w.recoveryAttempts==2 && w.recoverySuccesses==1 && w.timingErrors);
queued=10000;for(int i=0;i<2;++i){tick+=100;input.latest(w);}assert(w.timingErrors);
queued=10000;for(int i=0;i<2;++i){tick+=100;input.latest(w);}assert(!w.timingErrors && w.bias==1600);
tick+=300;input.latest(w);assert(w.timingErrors && w.pollErrors==1);
input.latest(w);assert(w.recoveryAttempts==2);tick=10000;input.latest(w);assert(w.recoveryAttempts==3 && w.timingErrors);
queued=10000;for(int i=0;i<2;++i){tick+=100;input.latest(w);}
assert(EspSctInput::pauseForUpload());int pausedStarts=starts,pausedReads=reads;
for(int i=0;i<20;++i){tick+=1000;input.latest(w);assert(w.pausedForUpload && w.timingErrors);}
assert(starts==pausedStarts && reads==pausedReads);
EspSctInput::resumeAfterUploadFailure();input.latest(w);assert(!w.pausedForUpload && w.timingErrors && starts==pausedStarts+1);
queued=10000;for(int i=0;i<2;++i){tick+=100;input.latest(w);}assert(w.timingErrors);
queued=10000;for(int i=0;i<2;++i){tick+=100;input.latest(w);}assert(!w.timingErrors);
EspSctInput::resumeAfterUploadFailure();assert(starts==pausedStarts+1);
}
'''
import subprocess
import tempfile
with tempfile.TemporaryDirectory(prefix="sct-dma-test-") as d:
    source=Path(d)/"test.cpp"
    binary=Path(d)/"test"
    source.write_text(h+s+t)
    subprocess.run(["c++", "-std=c++17", str(source), "-o", str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print("DMA recovery fault-injection checks passed")
