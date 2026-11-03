// EnvNode: see repository LICENSE.
#pragma once
#include <cstdint>
#include "ModuleSlot.h"
namespace EnvNode {
struct SdStorageConfiguration {
    bool enabled=false;
    ModuleSlot slot=ModuleSlot::A;
    bool systemLog=false;
    bool automatic=false,moduleConflict=false; // Runtime only; never stored in backups/NVS.
};
}
