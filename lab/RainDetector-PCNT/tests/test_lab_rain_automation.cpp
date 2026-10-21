// EnvNode-owned: see ../../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include <assert.h>
#include <initializer_list>
#include "LabRainAutomation.h"
using namespace RainLab;
struct Rig {
    LabRainAutomation a;
    uint64_t now = 0;
    AutoResult tick(float hz = 34000, unsigned heater = 0, unsigned dt = 250) {
        now += dt;
        return a.update(now, hz, heater);
    }
    void hold(unsigned n, float hz, unsigned heater) { while (n--) tick(hz, heater); }
    void ready(unsigned heater = 0) { a.enable(); hold(25, 34000, heater); }
};
int main() {
    AutoConfig c;
    assert(parseAutoConfig("AUTO CONFIG 1000 150 5 60 32000 500 10", c));
    assert(c.endSeconds == 60);
    for (const char* line : {"AUTO CONFIG 1000 150 5 29 32000 500 10",
        "AUTO CONFIG 1000 150 5 301 32000 500 10", "AUTO CONFIG 1000 150 0 60 32000 500 10",
        "AUTO CONFIG 1000 150 5 60 32000 32000 10", "AUTO CONFIG 1000 150 5 60 32000 500 0",
        "AUTO CONFIG 1000 150 5 60 32000 500 10 extra", "AUTO CONFIG 1000 150 5 60 32000 500",
        "AUTO CONFIG 1000150 5 60 32000 500 10", "AUTO CONFIG 1000 150 5 60 32000 500 1.5"}) {
        AutoConfig unchanged;
        assert(!parseAutoConfig(line, unchanged));
        assert(unchanged.endSeconds == 60);
    }
    assert(autoWord(" AUTO ON ", "ON") && !autoWord("AUTO ON extra", "ON"));
    Rig r;
    assert(!r.a.enabled() && !r.tick().heating);
    r.ready();
    assert(r.a.result().valid && r.a.result().rain == AutoRain::Unknown);
    auto wet = r.tick(33000);
    assert(wet.wetting && wet.heating && wet.rain == AutoRain::Active);
    // Full heater switch must not add five seconds of unobserved quiet time.
    r.hold(20, 30000, 100);
    assert(!r.a.result().valid && r.a.result().quietMs == 0);
    r.hold(239, 30000, 100);
    assert(r.a.result().rain == AutoRain::Active);
    r.hold(2, 30000, 100);
    assert(r.a.result().rain == AutoRain::EndEstimated && r.a.result().heating);
    assert(r.tick(29000, 100).wetting);
    assert(r.a.result().rain == AutoRain::Active);
    // Threshold reached: confirmation may stay inside hysteresis band.
    r.hold(5, 32010, 100);
    r.hold(20, 31920, 100);
    assert(!r.a.result().dry);
    r.tick(31499, 100);
    assert(r.a.result().dryMs == 0);
    r.hold(44, 31920, 100);
    assert(!r.a.result().dry); // Cannot start within hysteresis band.
    r.tick(32000, 100);
    r.hold(39, 31920, 100);
    assert(!r.a.result().dry);
    r.tick(31920, 100);
    assert(r.a.result().dry && !r.a.result().heating);
    // Cooling alone never starts a new heating cycle (slow drift, no edge).
    r.hold(25, 31920, 0);
    for (unsigned i = 0; i < 100; ++i) r.tick(31920 - i, 0);
    assert(!r.a.result().heating);
    assert(r.tick(30000, 0).wetting && r.a.result().heating);
    r.tick(NAN, 100);
    assert(!r.a.result().valid && r.a.result().rain == AutoRain::Unknown);
    assert(!r.a.result().heating && !r.a.result().dry && r.a.enabled());
    r.hold(300, 30000, 100);
    assert(r.a.result().rain == AutoRain::Unknown); // No end from missing history.
    assert(!r.a.result().heating && r.a.enabled());
    r.hold(25, 30000, 0);
    assert(!r.a.result().heating);
    assert(r.tick(29000, 0).wetting && r.a.result().heating);
    r.a.disable();
    assert(!r.a.enabled() && !r.a.result().heating);
    // Dry head at startup confirms dry without a heater cycle or rain claim.
    r.ready(); r.hold(300, 35000, 0);
    assert(r.a.result().dry && !r.a.result().heating);
    assert(r.a.result().rain == AutoRain::Unknown);
    Rig startup;
    startup.a.enable(); startup.hold(25,30000,0);
    assert(startup.a.result().heating && !startup.a.result().wetting);
    assert(startup.a.result().rain == AutoRain::Unknown);
    startup.hold(100,34000,100);
    assert(startup.a.result().dry && !startup.a.result().heating);
    // Partial PWM edges are explicitly experimental, wet-cycle dry needs full ON.
    Rig pwm; pwm.ready(30); assert(pwm.tick(33000,30).wetting);
    pwm.hold(300,35000,30); assert(!pwm.a.result().dry);
    // Clock reversal/gaps invalidate confirmation and stop a wet cycle.
    pwm.tick(35000,30,2000); assert(!pwm.a.result().valid);
    assert(pwm.a.result().rain == AutoRain::Unknown && !pwm.a.result().heating);
    pwm.a.update(1,35000,30); assert(!pwm.a.result().valid);
    AutoConfig minimum; minimum.endSeconds=30; assert(minimum.valid());
    minimum.endSeconds=300; assert(minimum.valid());
    assert(!pwm.a.configure(minimum));
    pwm.a.disable(); assert(pwm.a.configure(minimum));
    // Bounded maximum edge window at minimum supported sampling period.
    Rig dense; AutoConfig wide; wide.edgeMs=2000;
    assert(dense.a.configure(wide)); dense.a.enable();
    for(unsigned i=0;i<200;++i) dense.tick(34000,0,100);
    assert(dense.a.result().valid);
    assert(dense.tick(33000,0,100).wetting);
    // Check both permitted rain-end boundaries with no drying involved.
    for (unsigned seconds : {30U, 300U}) {
        Rig timed; AutoConfig cfg; cfg.endSeconds = seconds;
        assert(timed.a.configure(cfg)); timed.ready();
        assert(timed.tick(30000).wetting);
        timed.hold(seconds * 4 - 1, 30000, 0);
        assert(timed.a.result().rain == AutoRain::Active);
        timed.tick(30000);
        assert(timed.a.result().rain == AutoRain::EndEstimated);
    }
    Rig drift; drift.ready();
    for (unsigned i = 0; i < 500; ++i) drift.tick(34000 - i * 10);
    assert(!drift.a.result().heating); // Less than threshold in any edge window.

}
