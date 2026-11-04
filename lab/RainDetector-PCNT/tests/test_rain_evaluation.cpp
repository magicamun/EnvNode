// EnvNode: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <assert.h>
#include <limits>
#include "RainDetectorEvaluation.h"
using namespace EnvNode;
static_assert(sizeof(RainDetectorEvaluation) <= 256, "Keep evaluation RAM bounded");

int main() {
    RainEvaluationConfig config;
    config.heaterGuardMs = 1000;
    config.endConfirmationMs = 3000;
    config.trendTimeSeconds = 1;
    RainDetectorEvaluation core(config);
    RainEvaluation out;
    for (uint64_t t=0;t<=10000;t+=250) {
        out=core.update(t,34000 + (t%500 ? 4 : -4),0);
        assert(!out.newWetting && out.estimate==RainEstimate::Unknown);
    }
    out=core.update(10250,32000,0);
    assert(out.newWetting && out.estimate==RainEstimate::Active);
    // A stationary wet signal must never imply rain end by timeout alone.
    for(uint64_t t=10500;t<=30000;t+=250) {
        out=core.update(t,32000,0);
        assert(out.estimate==RainEstimate::Active);
    }
    for(uint64_t t=30250;t<=40000;t+=250)
        out=core.update(t,32000+static_cast<float>(t-30000)*0.03f,0);
    assert(out.estimate==RainEstimate::EndEstimated);
    out=core.update(40250,30000,0);
    assert(out.newWetting && out.estimate==RainEstimate::Active);
    // A heater transition cannot itself be a wetting event.
    out=core.update(40500,29000,100);
    assert(!out.valid && !out.newWetting && out.estimate==RainEstimate::Active);
    for(uint64_t t=40750;t<=42000;t+=250) {
        out=core.update(t,29000,100);assert(!out.newWetting);
    }
    out=core.update(44000,29000,100); // Gap: unknown, no stale rain state.
    assert(!out.valid && out.estimate==RainEstimate::Unknown);
    out=core.update(44250,std::numeric_limits<float>::quiet_NaN(),100);
    assert(!out.valid && out.estimate==RainEstimate::Unknown);
    out=core.update(44500,0,100);assert(!out.valid);
    out=core.update(44750,34000,50);assert(!out.valid);
    out=core.update(45000,34000,0);assert(!out.valid);
    out=core.update(45000,34000,0);assert(!out.valid); // Duplicate time
    out=core.update(250,34000,0);assert(!out.valid); // Reboot
    // Short pulse on an already wet surface: do not smooth it away.
    RainDetectorEvaluation pulse(config);
    for(uint64_t t=0;t<=10000;t+=250) pulse.update(t,7600,100);
    out=pulse.update(10250,7210,100);
    assert(out.newWetting);
    // Heating/measurement clock can run beyond 32-bit millisecond rollover.
    RainDetectorEvaluation longRun(config);
    const uint64_t base=5000000000ULL;
    for(uint64_t t=base;t<=base+10000;t+=250) longRun.update(t,34000,0);
    assert(longRun.update(base+10250,32000,0).newWetting);
    RainEvaluationConfig invalid=config;invalid.fastTimeSeconds=0;
    assert(!RainDetectorEvaluation(invalid).update(0,34000,0).valid);
    // Continuing rewetting must keep the state active despite positive ramps
    // between events. Ending the additions permits a later end estimate.
    RainEvaluationConfig repeatedConfig=config;
    repeatedConfig.endConfirmationMs=30000;
    RainDetectorEvaluation repeated(repeatedConfig);
    float lastFrequency=30000;
    for(uint64_t t=0;t<=100000;t+=250) {
        if(t<10000) lastFrequency=30000;
        else {
            const uint64_t cycle=(t-10000)/15000;
            const uint64_t phase=(t-10000)%15000;
            lastFrequency=30000-1000*static_cast<float>(cycle+1)
                +20*static_cast<float>(phase)/1000;
        }
        out=repeated.update(t,lastFrequency,100);
        if(t>=10000) assert(out.estimate==RainEstimate::Active);
    }
    for(uint64_t t=100250;t<=150000;t+=250)
        out=repeated.update(t,lastFrequency+20*static_cast<float>(t-100000)/1000,100);
    assert(out.estimate==RainEstimate::EndEstimated);

    // A recovery which reaches a stable plateau must be allowed to confirm;
    // requiring a positive slope forever would leave the estimate stuck active.
    RainEvaluationConfig plateauConfig=config;
    plateauConfig.endConfirmationMs=10000;
    RainDetectorEvaluation plateau(plateauConfig);
    for(uint64_t t=0;t<=10000;t+=250) plateau.update(t,34000,0);
    plateau.update(10250,32000,0);
    for(uint64_t t=10500;t<=50000;t+=250) {
        float hz=t<20000 ? 32000 : (t<25000 ? 32000+static_cast<float>(t-20000)*0.2f : 33000);
        out=plateau.update(t,hz,0);
    }
    assert(out.estimate==RainEstimate::EndEstimated);

}
