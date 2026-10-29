// EnvNode: see repository LICENSE.
#include "DuoRelayDescriptor.h"
#include "ModuleProfileTemplate.h"
namespace EnvNode {
namespace {
#include "DuoRelayProfile.generated.inc"
}
CompactCborStatus DuoRelayDescriptor::encode(
    const DuoRelayDescriptorManufacturingData& manufacturing,
    uint8_t* output, size_t capacity, size_t& size) {
    if (!output || !capacity) { size = 0; return CompactCborStatus::InvalidArgument; }
    return instantiateModuleProfile(DuoRelayProfileTemplate, sizeof(DuoRelayProfileTemplate),
        manufacturing, output, capacity, size)
        ? CompactCborStatus::Success : CompactCborStatus::InvalidArgument;
}
}
