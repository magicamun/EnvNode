// EnvNode: see repository LICENSE.
#include "FixtureRouting.h"
#include <cstring>
#include <algorithm>
namespace EnvNode { namespace Testing {
#include "FixtureRevision01.inc"
#include "Esp32Adapter03.inc"
#include "Controller02.inc"
namespace {
const ControllerProfile Controllers[]={{"controller-0.1","Test-Controller 0.1 (Legacy)",false,&FixtureRevision01},{"controller-0.2","Test-Controller 0.2 (TP2)",true,&FixtureRevision02}};
const AdapterProfile Adapters[]={{"input-0.1","Input-Adapter 0.1 (Legacy)","J4",FixtureRevision01.routing.socket},{"esp32-devkitc-0.3","ESP32 DevKitC Adapter 0.3","J2",Esp32Adapter03Inputs}};
const char* const Ids[2][2]={{"controller0.1-input0.1-module0.1","controller0.1-esp32-devkitc0.3"},{"controller0.2-input0.1","controller0.2-esp32-devkitc0.3"}};
}
FixtureSelection::FixtureSelection():definition_(FixtureRevision01) {}
const ControllerProfile& FixtureSelection::controller() const {return Controllers[controller_];}
const AdapterProfile& FixtureSelection::adapter() const {return Adapters[adapter_];}
bool FixtureSelection::selectCode(unsigned code) {return code<4&&select(Controllers[code/2].id,Adapters[code%2].id);}
bool FixtureSelection::select(const char* controller,const char* adapter) {
    if(!controller||!adapter)return false;
    int c=-1,a=-1;
    for(int i=0;i<2;++i){if(!strcmp(controller,Controllers[i].id))c=i;if(!strcmp(adapter,Adapters[i].id))a=i;}
    if(c<0||a<0)return false;
    definition_=*Controllers[c].wiring;controller_=unsigned(c);adapter_=unsigned(a);definition_.id=Ids[c][a];
    std::copy_n(Adapters[a].inputs,MaxInputs,definition_.routing.socket);
    return true;
}
const FixtureDefinition& fixtureRevision01() { return FixtureRevision01; }
bool resolveFixturePath(Pigtail pigtail, uint8_t contact, uint8_t input,
                        MuxAddress& source, MuxAddress& destination) {
    return resolveFixturePath(fixtureRevision01(),pigtail,contact,input,source,destination);
}
bool resolveFixturePath(const FixtureDefinition& fixture,Pigtail pigtail,uint8_t contact,uint8_t input,MuxAddress& source,MuxAddress& destination) {
    if (!input || input > MaxInputs || !contact) return false;
    const auto& routing = fixture.routing;
    const MuxAddress* outputs;
    size_t count;
    switch (pigtail) {
    case Pigtail::Gpio: outputs = routing.gpio; count = 4; break;
    case Pigtail::I2c: outputs = routing.i2c; count = 4; break;
    case Pigtail::Module: outputs = routing.module; count = MaxContacts; break;
    case Pigtail::Reset: outputs = routing.reset; count = 2; break;
    default: return false;
    }
    if (contact > count || outputs[contact-1].device > 1 || outputs[contact-1].channel > 15) return false;
    source = outputs[contact - 1]; destination = routing.socket[input - 1];
    return true;
}
} }
