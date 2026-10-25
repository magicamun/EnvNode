#pragma once

#include "HardwareDescriptorEnvelope.h"
#include "IIdentityStorage.h"

namespace EnvNode {

enum class HardwareDescriptorBank : uint8_t {
    A = 0,
    B = 1,
    None = 0xFF,
};

enum class HardwareDescriptorStoreStatus : uint8_t {
    Valid,
    NotProvisioned,
    StorageUnavailable,
    NoValidBank,
    BufferTooSmall,
    InvalidArgument,
    WriteFailed,
    VerificationFailed,
};

struct HardwareDescriptorReadResult {
    HardwareDescriptorStoreStatus status = HardwareDescriptorStoreStatus::NotProvisioned;
    HardwareDescriptorBank bank = HardwareDescriptorBank::None;
    HardwareDescriptorEnvelope envelope = {};
};

struct HardwareDescriptorWriteResult {
    HardwareDescriptorStoreStatus status = HardwareDescriptorStoreStatus::InvalidArgument;
    HardwareDescriptorBank bank = HardwareDescriptorBank::None;
    uint32_t generation = 0;
};

class HardwareDescriptorStore {
public:
    static constexpr uint16_t BankSize = 2048;
    static constexpr uint16_t BankAAddress = 0x0000;
    static constexpr uint16_t BankBAddress = 0x0800;
    static constexpr uint16_t MaximumPayloadSize =
        BankSize - HardwareDescriptorEnvelope::EncodedSize;

    explicit HardwareDescriptorStore(IIdentityStorage& storage);

    HardwareDescriptorReadResult read(uint8_t* payload, size_t capacity);
    HardwareDescriptorWriteResult write(
        HardwareDescriptorObjectKind objectKind,
        const uint8_t* payload,
        size_t payloadSize);

private:
    struct BankInspection {
        bool storageAvailable = true;
        bool hasMagic = false;
        bool valid = false;
        HardwareDescriptorEnvelope envelope = {};
    };

    BankInspection inspect(HardwareDescriptorBank bank);
    bool payloadMatches(
        HardwareDescriptorBank bank,
        const uint8_t* expected,
        size_t size);
    uint16_t bankAddress(HardwareDescriptorBank bank) const;

    IIdentityStorage& storage_;
};

const char* hardwareDescriptorStoreStatusName(HardwareDescriptorStoreStatus status);

} // namespace EnvNode
