#pragma once

#include <cstddef>
#include <type_traits>

#include "AM2302Sensor.h"
#include "BME280Sensor.h"
#include "HardwareResources.h"
#include "Logger.h"
#include "SimulatedHumiditySensor.h"
#include "SimulatedPressureSensor.h"
#include "SimulatedTemperatureSensor.h"
#include "SensorSlotConfiguration.h"
#include "RainGaugeSensor.h"
#include "I2CBusManager.h"

namespace WeatherStation {

enum class SensorFactoryResult {
    Created,
    NoRuntimeSensor,
    InvalidStorageIndex,
    StorageOccupied,
    UnknownImplementation,
    InvalidResource,
};

class SensorFactory {
public:
    SensorFactory(IMonotonicClock& monotonicClock, I2CBusManager& i2cBusManager, ILogger& logger);
    ~SensorFactory();

    ISensor* create(
        size_t storageIndex,
        const SensorSlotConfiguration& slot,
        SensorFactoryResult& result);
    void destroyAll();

private:
    using SensorStorage = typename std::aligned_union<0,
        SimulatedTemperatureSensor,
        SimulatedHumiditySensor,
        SimulatedPressureSensor,
        AM2302Sensor,
        BME280Sensor,
        RainGaugeSensor>::type;

    void destroy(size_t storageIndex);

    IMonotonicClock& monotonicClock_;
    I2CBusManager& i2cBusManager_;
    ILogger& logger_;
    SensorStorage storage_[MaxSensorSlotCount];
    SensorImplementation constructed_[MaxSensorSlotCount];
};

} // namespace WeatherStation
