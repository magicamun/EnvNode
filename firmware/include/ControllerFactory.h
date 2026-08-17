#pragma once

#include <cstddef>
#include <type_traits>

#include "BlinkController.h"
#include "ControllerSlotConfiguration.h"

namespace EnvNode {

enum class ControllerFactoryResult : uint8_t {
    Created,
    NoRuntimeController,
    InvalidStorageIndex,
    StorageOccupied,
    UnknownImplementation,
    InvalidConfiguration,
};

struct ControllerFactoryInstance {
    IController* controller = nullptr;
    BlinkController* blink = nullptr;
};

class ControllerFactory {
public:
    ControllerFactory(
        IOnOffActuatorResolver& actuatorResolver,
        IMonotonicClock& monotonicClock,
        ILogger& logger);
    ~ControllerFactory();

    ControllerFactoryInstance create(
        size_t storageIndex,
        const ControllerSlotConfiguration& slot,
        ControllerFactoryResult& result);
    void destroyAll();
    IOnOffActuatorResolver& actuatorResolver() const;
    IMonotonicClock& monotonicClock() const;

private:
    using ControllerStorage = typename std::aligned_union<0, BlinkController>::type;
    void destroy(size_t storageIndex);

    IOnOffActuatorResolver& actuatorResolver_;
    IMonotonicClock& monotonicClock_;
    ILogger& logger_;
    ControllerStorage storage_[MaxControllerSlotCount];
    ControllerImplementation constructed_[MaxControllerSlotCount];
};

} // namespace EnvNode
