#pragma once

#include <cstddef>

#include "ActuatorFactory.h"
#include "IOnOffActuatorResolver.h"

namespace EnvNode {

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
};

class ActuatorRuntime : public IOnOffActuatorResolver {
public:
    ActuatorRuntime(ActuatorFactory& factory, ILogger& logger);
    ~ActuatorRuntime();

    void initialize(const ActuatorSlotConfiguration* slots);
    bool rebuild(const ActuatorSlotConfiguration* slots);
    size_t runtimeCount() const;
    size_t availableCount() const;
    bool runtimeInfo(size_t index, ActuatorRuntimeInfo& info) const;
    IOnOffActuator* onOffActuator(ActuatorId id) override;
    const IOnOffActuator* onOffActuator(ActuatorId id) const;

private:
    struct RuntimeEntry {
        ActuatorRuntimeInfo info;
        IOnOffActuator* onOff = nullptr;
    };

    RuntimeEntry* findEntry(ActuatorId id);
    const RuntimeEntry* findEntry(ActuatorId id) const;
    bool validateComposition(const ActuatorSlotConfiguration* slots) const;
    bool constructComposition(
        ActuatorFactory& factory,
        const ActuatorSlotConfiguration* slots,
        RuntimeEntry* entries,
        size_t& runtimeCount) const;
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
    size_t runtimeCount_ = 0;
    size_t availableCount_ = 0;
    bool initialized_ = false;
};

} // namespace EnvNode
