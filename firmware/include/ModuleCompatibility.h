#pragma once

#include "BoardProfile.h"
#include "ModuleProfile.h"

namespace EnvNode {

enum class ModuleCompatibilityStatus : uint8_t {
    Compatible,
    ProfileUnavailable,
    SlotUnavailable,
    ResourceUnavailable,
    CapabilityMismatch,
};

struct ModuleCompatibilityResult {
    ModuleCompatibilityStatus status = ModuleCompatibilityStatus::ProfileUnavailable;
    const BoardModuleSlotCapability* slot = nullptr;
    ModuleConnectorResource failedResource = ModuleConnectorResource::I2C0;

    bool compatible() const { return status == ModuleCompatibilityStatus::Compatible; }
};

ModuleCompatibilityResult evaluateModuleCompatibility(
    const BoardProfile& board,
    ModuleSlot slot,
    const ModuleProfile* module);

const char* moduleCompatibilityStatusName(ModuleCompatibilityStatus status);

} // namespace EnvNode
