// EnvNode: see repository LICENSE.
#include "Esp32FixtureIo.h"
#include <Arduino.h>
namespace EnvNode {
void Esp32FixtureIo::selectDiagnostic(const Testing::DiagnosticPath& p) {
    const auto& f=definition();
    if((p.output.device>1 && p.output.device!=255)||p.output.channel>15||
        p.inputDevice < -1 || p.inputDevice>2 || p.inputChannel>15) {releaseDiagnostic();return;}
    for(auto pin:f.outputEnable) {digitalWrite(pin,HIGH);pinMode(pin,OUTPUT);}
    for(auto pin:f.inputEnable) {digitalWrite(pin,HIGH);pinMode(pin,OUTPUT);}
    pinMode(f.drive,INPUT);
    for(int bit=0;bit<4;++bit) {
        digitalWrite(f.outputSelect[bit],(p.output.channel>>bit)&1);pinMode(f.outputSelect[bit],OUTPUT);
        digitalWrite(f.inputSelect[bit],(p.inputChannel>>bit)&1);pinMode(f.inputSelect[bit],OUTPUT);
    }
    pinMode(f.sense,INPUT);analogReadResolution(12);analogSetPinAttenuation(f.sense,ADC_11db);
    digitalWrite(f.drive,p.high?HIGH:LOW);pinMode(f.drive,OUTPUT);
    if(p.output.device!=255)digitalWrite(f.outputEnable[p.output.device],LOW);
    if(p.inputDevice>=0)digitalWrite(f.inputEnable[p.inputDevice],LOW);
    configured_=true;
}
uint16_t Esp32FixtureIo::sampleDiagnostic() {return analogRead(definition().sense);}
void Esp32FixtureIo::releaseDiagnostic() {
    if(!configured_)return;
    const auto& f=definition();
    for(auto pin:f.outputEnable)digitalWrite(pin,HIGH);
    for(auto pin:f.inputEnable)digitalWrite(pin,HIGH);
    pinMode(f.drive,INPUT);configured_=false;
}
}
