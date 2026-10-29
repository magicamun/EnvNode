// EnvNode: see repository LICENSE.
#pragma once
#include "BoardIdentity.h"
#include "HardwareDescriptor.h"
#include "IIdentityStorage.h"
namespace EnvNode {
struct PortMapGpio { uint8_t pin = 0, capabilities = 0; };
struct PortMapI2c { uint8_t bus = 0, sda = 0, scl = 0; };
struct PortMapSlot { uint8_t slot = 0, address = 0, aux1 = 0, aux2 = 0, cs = 0; bool fiveVolt = false; };
struct BoardPortMap {
    uint8_t profileId = 0;
    BoardRevision revision = {};
    BoardSerialNumber serial = {};
    char name[96] = {};
    char productionBatch[65] = {};
    char productionDate[11] = {};
    DescriptorSemanticVersion minimumFirmware;
    PortMapGpio gpios[32]; size_t gpioCount = 0;
    PortMapI2c buses[2]; size_t busCount = 0;
    PortMapSlot slots[2]; size_t slotCount = 0;
    uint8_t spiClock = 0, spiMiso = 0, spiMosi = 0;
};
class BoardPortMapCodec {
public:
    static constexpr size_t MaximumBytes = 1024;
    static bool encode(const BoardPortMap&, uint8_t*, size_t, size_t&);
    static bool decode(const uint8_t*, size_t, BoardPortMap&);
};
enum class BoardPortMapStatus : uint8_t { Valid, NotProvisioned, Unavailable, Corrupt, InvalidMap, WriteFailed, VerificationFailed };
struct BoardPortMapResult { BoardPortMapStatus status = BoardPortMapStatus::NotProvisioned; uint32_t generation = 0; };
class BoardPortMapStore {
public:
    explicit BoardPortMapStore(IIdentityStorage& storage) : storage_(storage) {}
    BoardPortMapResult read(BoardPortMap&);
    BoardPortMapResult write(const BoardPortMap&);
private:
    struct Bank { bool available = true, present = false, valid = false, unsupported = false; uint16_t length = 0; uint32_t generation = 0; };
    Bank inspect(uint16_t address);
    IIdentityStorage& storage_;
};
const char* boardPortMapStatusName(BoardPortMapStatus);
}
