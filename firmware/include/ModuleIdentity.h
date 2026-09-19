#pragma once

#include <cstdint>

namespace EnvNode {

enum class ModuleProfileId : uint16_t {
    DuoRelay = 1,
    AnalogHydroPressure = 2,
};

struct ModuleRevision {
    uint8_t major;
    uint8_t minor;
};

using ModuleSerialNumber = uint32_t;

struct ModuleIdentity {
    ModuleProfileId profileId;
    ModuleRevision revision;
    ModuleSerialNumber serialNumber;
};

enum class ModuleIdentityStatus : uint8_t {
    Valid,
    StorageUnavailable,
    NotProvisioned,
    InvalidMagic,
    UnsupportedFormat,
    InvalidLength,
    InvalidCRC,
    UnknownModuleProfile,
    InvalidRevision,
    UnsupportedRevision,
    UnassignedSerial,
};

bool isKnownModuleProfileId(uint16_t encodedId);
bool decodeModuleProfileId(uint16_t encodedId, ModuleProfileId& profileId);
uint16_t encodeModuleProfileId(ModuleProfileId profileId);
ModuleIdentityStatus validateModuleIdentity(const ModuleIdentity& identity);

} // namespace EnvNode
