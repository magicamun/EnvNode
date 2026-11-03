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
// Resolve logical pins without driving hardware. Invalid requests never change outputs.
bool resolveFixturePath(Pigtail pigtail, uint8_t contact, uint8_t input,
                        MuxAddress& source, MuxAddress& destination);
} }
