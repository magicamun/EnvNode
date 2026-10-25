#pragma once

#include "CompactCborCodec.h"
#include "HardwareDescriptor.h"

namespace EnvNode {

enum class HardwareDescriptorDecodeStatus : uint8_t {
    Valid,
    InvalidCbor,
    MissingRequiredField,
    DuplicateOrUnorderedKey,
    InvalidValue,
    TooManyItems,
    ObjectKindMismatch,
    TrailingData,
};

class HardwareDescriptorCodec {
public:
    static HardwareDescriptorDecodeStatus decode(
        const uint8_t* payload,
        size_t payloadSize,
        HardwareDescriptorObjectKind expectedKind,
        HardwareDescriptor& descriptor);
};

const char* hardwareDescriptorDecodeStatusName(HardwareDescriptorDecodeStatus status);

} // namespace EnvNode
