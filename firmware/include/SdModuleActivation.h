// EnvNode: see repository LICENSE.
#pragma once
#include "ModuleDiscoveryService.h"
#include "SdStorageConfiguration.h"
namespace EnvNode {
bool sdModuleContractSupported(const HardwareDescriptor& descriptor);
void applyDiscoveredSdModule(const ModuleDiscoveryService& modules,SdStorageConfiguration& settings);
}
