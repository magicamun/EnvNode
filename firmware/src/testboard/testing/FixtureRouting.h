// EnvNode: see repository LICENSE.
#pragma once
#include "TestModel.h"
namespace EnvNode { namespace Testing {
struct FixtureDefinition {
    const char* id;
    FixtureRouting routing;
    uint8_t inputSelect[4], inputEnable[3], outputSelect[4], outputEnable[2];
    uint8_t sense, drive, provisioningSda, provisioningScl;
};
const FixtureDefinition& fixtureRevision01();
struct ControllerProfile {const char* id; const char* name; bool directTestDrive; const FixtureDefinition* wiring;};
struct AdapterProfile {const char* id; const char* name; const char* socketReference; const MuxAddress* inputs;};
// Runtime fixture composition. No GPIO effects and no implicit detection.
class FixtureSelection {
public:
    FixtureSelection();
    bool select(const char* controller,const char* adapter);
    bool selectCode(unsigned code);
    unsigned code() const {return controller_*2+adapter_;}
    const FixtureDefinition& definition() const {return definition_;}
    const ControllerProfile& controller() const;
    const AdapterProfile& adapter() const;
private:
    FixtureDefinition definition_;
    unsigned controller_=0,adapter_=0;
};
// Resolve logical pins without driving hardware. Invalid requests never change outputs.
bool resolveFixturePath(const FixtureDefinition& fixture,Pigtail pigtail,uint8_t contact,uint8_t input,
                        MuxAddress& source,MuxAddress& destination);
bool resolveFixturePath(Pigtail pigtail, uint8_t contact, uint8_t input,
                        MuxAddress& source, MuxAddress& destination);
} }
