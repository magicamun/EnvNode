// EnvNode: see repository LICENSE.
#include "FixtureRouting.h"
namespace EnvNode { namespace Testing {
#include "FixtureRevision01.inc"
const FixtureDefinition& fixtureRevision01() { return FixtureRevision01; }
bool resolveFixturePath(Pigtail pigtail, uint8_t contact, uint8_t input,
                        MuxAddress& source, MuxAddress& destination) {
    if (!input || input > MaxInputs || !contact) return false;
    const auto& routing = fixtureRevision01().routing;
    const MuxAddress* outputs;
    size_t count;
    switch (pigtail) {
    case Pigtail::Gpio: outputs = routing.gpio; count = 4; break;
    case Pigtail::I2c: outputs = routing.i2c; count = 4; break;
    case Pigtail::Module: outputs = routing.module; count = MaxContacts; break;
    default: return false;
    }
    if (contact > count) return false;
    source = outputs[contact - 1]; destination = routing.socket[input - 1];
    return true;
}
} }
