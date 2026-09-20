#pragma once

#include <cstddef>
#include <cstdint>

#include "BoardProfile.h"
#include "HardwareDescriptor.h"

namespace EnvNode {

enum class ModuleDeviceInventoryStatus : uint8_t {
    Ready,
    UnsupportedDeviceKind,
    UnsupportedDriver,
    MissingCapability,
    MissingBinding,
    UnknownBindingTarget,
    ResourceUnavailable,
    InvalidParameters,
};

struct ModuleDeviceBindingResolution {
    DescriptorTextView name = {};
    DescriptorTextView target = {};
    DescriptorTextView logicalResource = {};
    HardwareDescriptorResourceKind kind = HardwareDescriptorResourceKind::Gpio;
    bool resolved = false;
    GpioResource gpio = GpioResource();
    I2CBus i2cBus = I2CBus::I2C0;
};

struct ModuleDeviceInventoryEntry {
    ModuleSlot slot = ModuleSlot::A;
    DescriptorTextView id = {};
    HardwareDescriptorDeviceKind kind = HardwareDescriptorDeviceKind::Sensor;
    DescriptorContract driver = {};
    DescriptorCapabilitySet capabilities = {};
    ModuleDeviceBindingResolution bindings[MaximumDescriptorBindings] = {};
    size_t bindingCount = 0;
    bool hasActiveLevel = false;
    bool activeLevelHigh = false;
    bool hasSafeLevel = false;
    bool safeLevelHigh = false;
    ModuleDeviceInventoryStatus status = ModuleDeviceInventoryStatus::UnsupportedDriver;
};

bool deriveModuleDeviceInventory(
    const HardwareDescriptor& descriptor,
    const BoardProfile& board,
    ModuleSlot slot,
    ModuleDeviceInventoryEntry* entries,
    size_t capacity,
    size_t& count);

const char* moduleDeviceKindName(HardwareDescriptorDeviceKind kind);
const char* moduleDeviceDriverName(const DescriptorContract& driver);
const char* moduleDeviceInventoryStatusName(ModuleDeviceInventoryStatus status);

} // namespace EnvNode
