// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
#include <unity.h>
#include <Arduino.h>
#include "Sct013Sensor.h"
#include "UnitConverter.h"
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
void HardwareSerial::println(const char*) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

struct Input : ISctInput {
    SctWindow w;
    bool begin(uint8_t pin,uint32_t hz) override { return pin==35 && hz==200; }
    bool latest(SctWindow& out) override { out=w;return w.revision!=0; }
};
struct Clock : IMonotonicClock { uint32_t t=1; uint32_t nowMs() const override{return t;} };
struct Sink : IMeasurementSink { Measurement m; void emit(const Measurement& v) override {m=v;} };
void setUp(){} void tearDown(){}
void test_calibrated_gate_and_faults() {
    Input i; Clock c; Sink s; Sct013Settings cfg;
    cfg.cal=1.48f;cfg.offMa=90;cfg.onMa=120;
    Sct013Sensor sensor(1,35,cfg,i,c); sensor.begin();
    auto sample=[&](float mv,bool valid,float expected) {
        ++i.w.revision;i.w.endMs=c.t;i.w.filteredMv=mv;
        sensor.sample(s);TEST_ASSERT_EQUAL(valid,s.m.valid);
        if(valid){float v=-1; TEST_ASSERT_TRUE(s.m.value.tryGetFloatingPoint(v));TEST_ASSERT_FLOAT_WITHIN(.0001f,expected,v);}
    };
    i.w.bias=1600;i.w.minimum=1500;i.w.maximum=1700;
    sample(5,true,0); sample(7,true,0); sample(10,true,.148f); sample(7,true,.1036f);
    i.w.timingErrors=1;sample(1,false,0);i.w.timingErrors=0;sample(7,true,.1036f);
    sample(5,true,0);sample(85.8f,true,1.26984f);
    c.t+=1600;sensor.sample(s);TEST_ASSERT_FALSE(s.m.valid);
    i.w.minimum=142;sample(10,false,0);
    TEST_ASSERT_EQUAL_STRING("A",UnitConverter::symbol(measurementTypeMetadata(s.m.type).canonicalUnit));
}
void test_interval_boundaries() {
    SctTiming t;
    for(int us : {249,250,500,1000,1001,3500}) t.interval(us);
    TEST_ASSERT_EQUAL_UINT32(1,t.shortIntervals);
    TEST_ASSERT_EQUAL_UINT32(2,t.longIntervals);
    TEST_ASSERT_EQUAL_INT(3500,t.maxGapUs);
}
int main(){UNITY_BEGIN();RUN_TEST(test_interval_boundaries);RUN_TEST(test_calibrated_gate_and_faults);return UNITY_END();}
