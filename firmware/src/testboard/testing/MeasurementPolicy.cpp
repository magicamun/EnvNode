// EnvNode: see repository LICENSE.
#include "MeasurementPolicy.h"
#include <cstring>
namespace EnvNode { namespace Testing {
namespace {
const DirectedRule Rules[]={{"GND","I2C0_SDA","I2C0_SDA"},{"GND","I2C0_SCL","I2C0_SCL"}};
// Weather 0.3: measured GND -> SYS rail conducts, reverse blocks. D2 carries
// that forward path to input 39; reverse drive comes from a SYS pigtail pin.
const DirectedRule WeatherRules[]={
    {"GND","I2C0_SDA","I2C0_SDA"},{"GND","I2C0_SCL","I2C0_SCL"},
    {"GND","Net-(D2-K)","+3V3_SYS"}
};
const MeasurementPolicy WeatherPolicy={"weather-ground-clamps-v2",WeatherRules,3};
// New reset headers are passive. EEPROM/reference-diode topology is unchanged
// on reviewed Mini 0.10.0 and Weather 0.4; physical acceptance remains open.
const char* const ReviewedProfiles[]={"weather-0.2-pcb0.2.1","mini-0.8-pcb0.8.1","weather-0.3-pcb0.3.0","mini-0.9-pcb0.9.0","mini-0.10-pcb0.10.0","weather-0.4-pcb0.4"};
const MeasurementPolicy Policy={"eeprom-ground-clamp-v1",Rules,2};
bool equal(const char* a,const char* b){return a&&b&&!strcmp(a,b);}
}
const MeasurementPolicy* measurementPolicy(const BoardDefinition& b) {
    // New profiles never inherit semiconductor exceptions just by joining BoardCatalog.
    bool reviewed=false;
    for(const char* id:ReviewedProfiles)reviewed|=equal(b.id,id);
    if(!reviewed)return nullptr;
    const auto* c=findBoard(b.id);
    if(!c||!equal(b.identityRevision,c->identityRevision)||!equal(b.pcbRevision,c->pcbRevision)||
        !b.socket||!b.connectors||b.inputCount!=c->inputCount||b.count!=c->count)return nullptr;
    for(size_t i=0;i<b.inputCount;++i)
        if(b.socket[i].number!=c->socket[i].number||!equal(b.socket[i].net,c->socket[i].net))return nullptr;
    for(size_t i=0;i<b.count;++i){const auto& x=b.connectors[i];const auto& y=c->connectors[i];
        if(!equal(x.id,y.id)||x.pigtail!=y.pigtail||x.count!=y.count||!x.contacts)return nullptr;
        for(size_t j=0;j<x.count;++j)if(x.contacts[j].number!=y.contacts[j].number||
            x.contacts[j].expected!=y.contacts[j].expected||!equal(x.contacts[j].net,y.contacts[j].net))return nullptr;
    }
    return (equal(b.id,"weather-0.3-pcb0.3.0")||equal(b.id,"weather-0.4-pcb0.4"))?&WeatherPolicy:&Policy;
}
int forwardRule(const MeasurementPolicy* p,const char* a,const char* b){
    if(p&&a&&b)for(size_t i=0;i<p->count;++i)if(equal(a,p->rules[i].from)&&equal(b,p->rules[i].to))return int(i);
    return -1;
}
int reverseRule(const MeasurementPolicy* p,const char* a,const char* b){
    if(p&&a&&b)for(size_t i=0;i<p->count;++i)
        if(equal(a,p->rules[i].reverseSource)&&equal(b,p->rules[i].from))return int(i);
    return -1;
}
} }
