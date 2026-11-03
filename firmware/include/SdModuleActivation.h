// EnvNode: see repository LICENSE.
#pragma once
#include "ModuleDiscoveryService.h"
#include "SdModuleContract.h"
#include "SdStorageConfiguration.h"
namespace EnvNode {
void applyDiscoveredSdModule(const ModuleDiscoveryService& modules,SdStorageConfiguration& settings);
}
