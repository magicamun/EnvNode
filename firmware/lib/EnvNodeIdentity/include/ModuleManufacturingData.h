// EnvNode: see repository LICENSE.
#pragma once
#include <cstdint>
namespace EnvNode {
struct ModuleManufacturingData {
    uint8_t instanceId[16] = {};
    const char* serialNumber = nullptr;
    const char* productionBatch = nullptr;
    const char* productionDate = nullptr;
};
}
