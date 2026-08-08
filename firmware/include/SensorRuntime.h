#pragma once

#include "IConfigurationService.h"
#include "ISensorRuntime.h"
#include "SensorFactory.h"
#include "SensorManager.h"

namespace WeatherStation {

class SensorRuntime : public ISensorRuntime {
public:
    SensorRuntime(
        IConfigurationService& configurationService,
        SensorFactory& firstFactory,
        SensorFactory& secondFactory,
        SensorManager& sensorManager);

    bool initialize(size_t& activeSensorCount, const char*& failureReason);
    bool rebuild(size_t& activeSensorCount, const char*& failureReason) override;

private:
    bool construct(
        SensorFactory& factory,
        const SensorSlotConfiguration* slots,
        ISensor** sensors,
        size_t& activeSensorCount,
        const char*& failureReason);
    bool registerComposition(
        const SensorSlotConfiguration* slots,
        ISensor* const* sensors,
        size_t& activeSensorCount,
        const char*& failureReason);
    void snapshotActiveConfiguration(const SensorSlotConfiguration* slots, ISensor* const* sensors);

    IConfigurationService& configurationService_;
    SensorFactory* activeFactory_;
    SensorFactory* inactiveFactory_;
    SensorManager& sensorManager_;
    SensorSlotConfiguration activeSlots_[MaxSensorSlotCount];
    ISensor* activeSensors_[MaxSensorSlotCount] = {};
};

} // namespace WeatherStation
