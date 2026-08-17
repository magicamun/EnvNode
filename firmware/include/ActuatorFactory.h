#pragma once

#include <cstddef>
#include <type_traits>

#include "ActuatorSlotConfiguration.h"
#include "GpioOnOffActuator.h"
#include "Logger.h"

namespace EnvNode {

enum class ActuatorFactoryResult : uint8_t {
    Created,
    NoRuntimeActuator,
    InvalidStorageIndex,
    StorageOccupied,
    UnknownImplementation,
    InvalidResource,
};

struct ActuatorFactoryInstance {
    IOnOffActuator* onOff = nullptr;
};

class ActuatorFactory {
public:
    explicit ActuatorFactory(ILogger& logger);
    ~ActuatorFactory();

    ActuatorFactoryInstance create(
        size_t storageIndex,
        const ActuatorSlotConfiguration& slot,
        ActuatorFactoryResult& result);
    void destroyAll();

private:
    using ActuatorStorage = typename std::aligned_union<0, GpioOnOffActuator>::type;

    void destroy(size_t storageIndex);

    ILogger& logger_;
    ActuatorStorage storage_[MaxActuatorSlotCount];
    ActuatorImplementation constructed_[MaxActuatorSlotCount];
};

} // namespace EnvNode
