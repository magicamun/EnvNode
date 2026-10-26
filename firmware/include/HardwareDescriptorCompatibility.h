#pragma once

#include "BoardProfile.h"
#include "HardwareDescriptor.h"
#include "HardwareDescriptorCompatibilityTypes.h"

namespace EnvNode {

HardwareDescriptorCompatibilityResult evaluateModuleDescriptorCompatibility(
    const HardwareDescriptor& descriptor,
    DescriptorSemanticVersion firmwareVersion,
    const BoardProfile& board,
    ModuleSlot slot);

DescriptorSemanticVersion currentFirmwareDescriptorVersion();


} // namespace EnvNode
