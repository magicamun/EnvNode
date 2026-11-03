// EnvNode: see repository LICENSE.
#pragma once
#include <cstddef>
#include "ModuleManufacturingData.h"
namespace EnvNode {
// Preserve arbitrary driver/resource definitions; replace only manufacturing identity.
bool instantiateModuleProfile(const uint8_t* bytes,size_t size,
    const ModuleManufacturingData& manufacturing,
    uint8_t* output,size_t capacity,size_t& outputSize);
}
