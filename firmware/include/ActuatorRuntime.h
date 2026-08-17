#pragma once

#include <cstddef>

#include "ActuatorFactory.h"

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

class ActuatorRuntime {
public:
    ActuatorRuntime(ActuatorFactory& factory, ILogger& logger);

    void initialize(const ActuatorSlotConfiguration* slots);
    size_t runtimeCount() const;
    size_t availableCount() const;
    bool runtimeInfo(size_t index, ActuatorRuntimeInfo& info) const;
    IOnOffActuator* onOffActuator(ActuatorId id);
    const IOnOffActuator* onOffActuator(ActuatorId id) const;

private:
    struct RuntimeEntry {
        ActuatorRuntimeInfo info;
        IOnOffActuator* onOff = nullptr;
    };

    RuntimeEntry* findEntry(ActuatorId id);
    const RuntimeEntry* findEntry(ActuatorId id) const;

    ActuatorFactory& factory_;
    ILogger& logger_;
    RuntimeEntry entries_[MaxActuatorSlotCount];
    size_t runtimeCount_ = 0;
    size_t availableCount_ = 0;
    bool initialized_ = false;
};

} // namespace EnvNode
