#pragma once

#include <cstddef>

#include "ActuatorFactory.h"
#include "IOnOffActuatorResolver.h"
#include "ModuleSlot.h"

namespace EnvNode {

enum class ActuatorRuntimeOrigin : uint8_t {
    SavedConfiguration,
    ModuleDescriptor,
};

struct AutomaticActuatorDefinition {
    ModuleSlot moduleSlot = ModuleSlot::A;
    uint8_t moduleInstanceFingerprint[8] = {};
    bool hasModuleInstanceId = false;
    char deviceId[MaxActuatorSlotNameLength + 1] = {};
    char name[MaxActuatorSlotNameLength + 1] = {};
    ActuatorImplementation implementation = ActuatorImplementation::None;
    HardwareResourceAssignment hardware;
};

struct ActuatorRuntimeInfo {
    ActuatorId id = InvalidActuatorId;
    char name[MaxActuatorSlotNameLength + 1] = {};
    ActuatorImplementation implementation = ActuatorImplementation::None;
    ActuatorCapability capabilities = ActuatorCapability::None;
    HardwareResourceAssignment hardware;
    ActuatorFactoryResult constructionResult = ActuatorFactoryResult::NoRuntimeActuator;
    bool initializationAttempted = false;
    ActuatorOperationResult initializationResult = ActuatorOperationResult::NotInitialized;
    bool available = false;
    ActuatorRuntimeOrigin origin = ActuatorRuntimeOrigin::SavedConfiguration;
    ModuleSlot moduleSlot = ModuleSlot::A;
    char descriptorDeviceId[MaxActuatorSlotNameLength + 1] = {};
};

class ActuatorRuntime : public IOnOffActuatorResolver {
public:
    ActuatorRuntime(ActuatorFactory& factory, ILogger& logger);
    ~ActuatorRuntime();

    bool configureAutomaticActuators(
        const AutomaticActuatorDefinition* definitions,
        size_t count);
    void initialize(const ActuatorSlotConfiguration* slots);
    bool rebuild(const ActuatorSlotConfiguration* slots);
    bool moduleOwnsHardware(const HardwareResourceAssignment& hardware) const;
    size_t runtimeCount() const;
    size_t availableCount() const;
    bool runtimeInfo(size_t index, ActuatorRuntimeInfo& info) const;
    bool moduleReference(ActuatorId id, ModuleActuatorReference& reference) const override;
    IOnOffActuator* onOffActuator(ActuatorId id) override;
    IOnOffActuator* onOffActuator(
        const ModuleActuatorReference& reference) override;
    const IOnOffActuator* onOffActuator(ActuatorId id) const;
    ILevelActuator* levelActuator(ActuatorId id);
    const ILevelActuator* levelActuator(ActuatorId id) const;

private:
    struct RuntimeEntry {
        ActuatorRuntimeInfo info;
        IOnOffActuator* onOff = nullptr;
        ILevelActuator* level = nullptr;
        const AutomaticActuatorDefinition* automaticOrigin = nullptr;
    };

    RuntimeEntry* findEntry(ActuatorId id);
    const RuntimeEntry* findEntry(ActuatorId id) const;
    bool validateComposition(const ActuatorSlotConfiguration* slots) const;
    bool constructComposition(
        ActuatorFactory& factory,
        const ActuatorSlotConfiguration* slots,
        const AutomaticActuatorDefinition* const* origins,
        RuntimeEntry* entries,
        size_t& runtimeCount) const;
    bool composeEffectiveSlots(const ActuatorSlotConfiguration* slots);
    bool initializeComposition(
        RuntimeEntry* entries,
        size_t runtimeCount,
        size_t& availableCount) const;
    void shutdownComposition(RuntimeEntry* entries, size_t runtimeCount) const;

    ActuatorFactory* activeFactory_;
    ActuatorFactory* inactiveFactory_;
    ActuatorFactory secondaryFactory_;
    ILogger& logger_;
    RuntimeEntry entries_[MaxActuatorSlotCount];
    AutomaticActuatorDefinition* automaticActuators_ = nullptr;
    const AutomaticActuatorDefinition* effectiveOrigins_[MaxActuatorSlotCount] = {};
    ActuatorSlotConfiguration* effectiveSlots_ = nullptr;
    size_t automaticActuatorCount_ = 0;
    size_t runtimeCount_ = 0;
    size_t availableCount_ = 0;
    bool initialized_ = false;
};

} // namespace EnvNode
