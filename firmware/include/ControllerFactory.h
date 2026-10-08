#pragma once

#include <cstddef>
#include <type_traits>

#include "BlinkController.h"
#include "ControllerSlotConfiguration.h"
#include "ThresholdController.h"
#include "SelectorController.h"

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
    ThresholdController* threshold = nullptr;
    SelectorController* selector = nullptr;
};

class ControllerFactory {
public:
    ControllerFactory(
        IMeasurementResolver& measurementResolver,
        IOnOffActuatorResolver& actuatorResolver,
        IMonotonicClock& monotonicClock,
        ILogger& logger, const IEnumValueReader* values = nullptr);
    ~ControllerFactory();

    ControllerFactoryInstance create(
        size_t storageIndex,
        const ControllerSlotConfiguration& slot,
        ControllerFactoryResult& result);
    void destroyAll();
    IMeasurementResolver& measurementResolver() const;
    IOnOffActuatorResolver& actuatorResolver() const;
    IMonotonicClock& monotonicClock() const;
    const IEnumValueReader* values() const { return values_; }

private:
    using ControllerStorage = typename std::aligned_union<
        0, BlinkController, ThresholdController, SelectorController>::type;
    static_assert(sizeof(ControllerStorage) >= sizeof(BlinkController),
        "Controller storage must fit BlinkController");
    static_assert(sizeof(ControllerStorage) >= sizeof(ThresholdController),
        "Controller storage must fit ThresholdController");
    static_assert(alignof(ControllerStorage) >= alignof(BlinkController),
        "Controller storage must align BlinkController");
    static_assert(alignof(ControllerStorage) >= alignof(ThresholdController),
        "Controller storage must align ThresholdController");
    static_assert(sizeof(ControllerStorage) >= sizeof(SelectorController),
        "Controller storage must fit SelectorController");
    static_assert(alignof(ControllerStorage) >= alignof(SelectorController),
        "Controller storage must align SelectorController");
    void destroy(size_t storageIndex);

    IMeasurementResolver& measurementResolver_;
    IOnOffActuatorResolver& actuatorResolver_;
    IMonotonicClock& monotonicClock_;
    ILogger& logger_;
    const IEnumValueReader* values_;
    ControllerStorage storage_[MaxControllerSlotCount];
    ControllerImplementation constructed_[MaxControllerSlotCount];
};

} // namespace EnvNode
