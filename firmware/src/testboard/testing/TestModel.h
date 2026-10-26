// EnvNode: see repository LICENSE.
#pragma once
#include <cstddef>
#include <cstdint>
namespace EnvNode { namespace Testing {
constexpr size_t SocketPins = 38, MaxContacts = 14, MaxConnectors = 32;
enum class Pigtail : uint8_t { Gpio, I2c, Module };
struct Contact { uint8_t number; const char* net; float x, y; uint64_t expected; };
struct SocketContact { uint8_t number; const char* net; float x, y; };
struct Connector { const char* id; Pigtail pigtail; uint8_t count; const Contact* contacts; };
struct BoardDefinition {
    const char* id; const char* name; const char* identityRevision; const char* pcbRevision;
    const SocketContact* socket; const Connector* connectors; size_t count;
};
const BoardDefinition& weatherBoard();
// Fixture routing belongs to the fixture revision, never to a DUT profile.
struct MuxAddress { uint8_t device; uint8_t channel; };
struct FixtureRouting {
    MuxAddress socket[SocketPins];
    MuxAddress gpio[4], i2c[4], module[MaxContacts];
};
// Physical routing is intentionally not populated until the PCB wiring is final.
class ITestFixture {
public:
    enum class Sample { Busy, Ready, Error };
    virtual ~ITestFixture() = default;
    // 1-based connector contact and socket pad numbers. Switch only in this layer.
    virtual bool select(Pigtail pigtail, uint8_t contact, uint8_t socket) = 0;
    virtual Sample read(bool& connected) = 0;
    virtual void release() = 0;
};
} }
