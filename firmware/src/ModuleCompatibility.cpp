#include "ModuleCompatibility.h"

namespace EnvNode {
namespace {

const BoardGpioCapability* gpioCapability(
    const BoardProfile& board,
    GpioResource resource) {
    for (size_t index = 0; index < board.gpioCount; ++index) {
        if (board.gpios[index].resource.number == resource.number) {
            return &board.gpios[index];
        }
    }
    return nullptr;
}

bool hasI2CBus(const BoardProfile& board, I2CBus bus) {
    for (size_t index = 0; index < board.i2cBusCount; ++index) {
        if (board.i2cBuses[index].bus == bus) return true;
    }
    return false;
}

GpioResource connectorGpio(
    const BoardModuleSlotCapability& slot,
    ModuleConnectorResource resource) {
    switch (resource) {
        case ModuleConnectorResource::AuxGpio1: return slot.auxGpio1;
        case ModuleConnectorResource::AuxGpio2: return slot.auxGpio2;
        case ModuleConnectorResource::SPI: return slot.spiChipSelect;
        default: return GpioResource();
    }
}

} // namespace

ModuleCompatibilityResult evaluateModuleCompatibility(
    const BoardProfile& board,
    ModuleSlot slotId,
    const ModuleProfile* module) {
    ModuleCompatibilityResult result;
    if (module == nullptr) return result;

    result.slot = boardModuleSlot(board, slotId);
    if (result.slot == nullptr) {
        result.status = ModuleCompatibilityStatus::SlotUnavailable;
        return result;
    }

    for (size_t index = 0; index < module->requirementCount; ++index) {
        const ModuleResourceRequirement& requirement = module->requirements[index];
        result.failedResource = requirement.resource;
        switch (requirement.resource) {
            case ModuleConnectorResource::I2C0:
                if (!hasI2CBus(board, I2CBus::I2C0)) {
                    result.status = ModuleCompatibilityStatus::ResourceUnavailable;
                    return result;
                }
                break;
            case ModuleConnectorResource::I2C1:
                if (!hasI2CBus(board, I2CBus::I2C1)) {
                    result.status = ModuleCompatibilityStatus::ResourceUnavailable;
                    return result;
                }
                break;
            case ModuleConnectorResource::FiveVoltSupply:
                if (!result.slot->fiveVoltSupplyAvailable) {
                    result.status = ModuleCompatibilityStatus::ResourceUnavailable;
                    return result;
                }
                break;
            case ModuleConnectorResource::AuxGpio1:
            case ModuleConnectorResource::AuxGpio2:
            case ModuleConnectorResource::SPI: {
                const BoardGpioCapability* capability = gpioCapability(
                    board, connectorGpio(*result.slot, requirement.resource));
                if (capability == nullptr) {
                    result.status = ModuleCompatibilityStatus::ResourceUnavailable;
                    return result;
                }
                if (!hasGpioCapabilities(
                        capability->capabilities,
                        requirement.requiredGpioCapabilities)) {
                    result.status = ModuleCompatibilityStatus::CapabilityMismatch;
                    return result;
                }
                break;
            }
        }
    }

    result.status = ModuleCompatibilityStatus::Compatible;
    return result;
}

const char* moduleCompatibilityStatusName(ModuleCompatibilityStatus status) {
    switch (status) {
        case ModuleCompatibilityStatus::Compatible: return "Compatible";
        case ModuleCompatibilityStatus::ProfileUnavailable: return "ProfileUnavailable";
        case ModuleCompatibilityStatus::SlotUnavailable: return "SlotUnavailable";
        case ModuleCompatibilityStatus::ResourceUnavailable: return "ResourceUnavailable";
        case ModuleCompatibilityStatus::CapabilityMismatch: return "CapabilityMismatch";
        default: return "Unknown";
    }
}

} // namespace EnvNode
