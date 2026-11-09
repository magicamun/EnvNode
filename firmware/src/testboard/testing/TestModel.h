// EnvNode: see repository LICENSE.
#pragma once
#include <cstddef>
#include <cstdint>
namespace EnvNode { namespace Testing {
constexpr size_t SocketPins = 38, MaxInputs = 40, MaxContacts = 14, MaxConnectors = 32;
constexpr uint8_t Mainboard3V3Input = 39, MainboardGroundInput = 40;
enum class Pigtail : uint8_t { Gpio, I2c, Module, Reset };
struct Contact { uint8_t number; const char* net; float x, y; uint64_t expected; };
struct SocketContact { uint8_t number; const char* net; float x, y; };
struct Connector { const char* id; Pigtail pigtail; uint8_t count; const Contact* contacts; };
struct BoardDefinition {
    const char* id; const char* name; const char* identityRevision; const char* pcbRevision;
    const SocketContact* socket; const Connector* connectors; size_t count;
    size_t inputCount; // 38 socket contacts; newer profiles may add reference inputs 39/40.
};
const BoardDefinition& weatherBoard();
const BoardDefinition& miniBoard();
size_t boardCount();
const BoardDefinition* boardAt(size_t index);
const BoardDefinition* findBoard(const char* id);
// Fixture routing belongs to the fixture revision, never to a DUT profile.
struct MuxAddress { uint8_t device; uint8_t channel; };
struct FixtureRouting {
    MuxAddress socket[MaxInputs];
    MuxAddress gpio[4], i2c[4], module[MaxContacts];
    MuxAddress reset[2]; // Unsupported on legacy controllers.
};
enum class MeasurementReason : uint8_t { None, Unavailable, SelectionFailed, Timeout, LowNotLow, HighAmbiguous };
struct MeasurementTrace {
    uint16_t low[8]={},high[8]={};
    uint8_t lowCount=0,highCount=0;
    MeasurementReason reason=MeasurementReason::Unavailable;
};
// Physical I/O remains inactive until assembled-fixture validation.
class ITestFixture {
public:
    enum class Sample { Busy, Ready, Error };
    virtual ~ITestFixture() = default;
    // 1-based connector contact and logical input numbers. Switch only in this layer.
    virtual bool select(Pigtail pigtail, uint8_t contact, uint8_t socket) = 0;
    virtual Sample read(bool& connected) = 0;
    virtual void release() = 0;
    virtual bool trace(MeasurementTrace&) const { return false; }
};
} }
