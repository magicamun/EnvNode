#pragma once

#include <cstddef>
#include <type_traits>
#include <memory>

#include "AM2302Sensor.h"
#include "BME280Sensor.h"
#include "SHT4xSensor.h"
#include "SHTC3Sensor.h"
#include "SolarPositionSensor.h"
#include "HardwareResources.h"
#include "Logger.h"
#include "SimulatedHumiditySensor.h"
#include "SimulatedPressureSensor.h"
#include "SimulatedTemperatureSensor.h"
#include "SensorSlotConfiguration.h"
#include "RainGaugeSensor.h"
#include "RainDetectorSensor.h"
#include "Esp32RainFrequencyInput.h"
#include "I2CBusManager.h"

namespace EnvNode {

enum class SensorFactoryResult {
    Created,
    NoRuntimeSensor,
    InvalidStorageIndex,
    StorageUnavailable,
    StorageOccupied,
    UnknownImplementation,
    InvalidResource,
};

class SensorFactory {
public:
    SensorFactory(IMonotonicClock& monotonicClock, I2CBusManager& i2cBusManager, ILogger& logger,
        const IConfigurationService* configuration = nullptr, const ITimeService* time = nullptr);
    ~SensorFactory();

    ISensor* create(
        size_t storageIndex,
        const SensorSlotConfiguration& slot,
        SensorFactoryResult& result);
    void destroyAll();

private:
    struct RainDetectorInstance {
        Esp32RainFrequencyInput input;
        RainDetectorSensor sensor;
        RainDetectorInstance(SensorId id, uint8_t gpio, IMonotonicClock& clock)
            : input(), sensor(id,gpio,input,clock) {}
    };
    using SensorStorage = typename std::aligned_union<0,
        SimulatedTemperatureSensor,
        SimulatedHumiditySensor,
        SimulatedPressureSensor,
        AM2302Sensor,
        BME280Sensor,
        SHT4xSensor,
        SHTC3Sensor,
        SolarPositionSensor,
        RainGaugeSensor, RainDetectorInstance>::type;

    void destroy(size_t storageIndex);

    IMonotonicClock& monotonicClock_;
    I2CBusManager& i2cBusManager_;
    ILogger& logger_;
    const IConfigurationService* configuration_;
    const ITimeService* time_;
    std::unique_ptr<SensorStorage[]> storage_;
    SensorImplementation constructed_[MaxSensorSlotCount];
};

} // namespace EnvNode
