#pragma once

#include "HardwareResources.h"
#include "SensorId.h"
#include "SensorImplementationRegistry.h"
#include "SensorSchedule.h"

namespace WeatherStation {

struct SimulatedTemperatureConfiguration {};
struct SimulatedHumidityConfiguration {};
struct SimulatedPressureConfiguration {};

struct AM2302Configuration {
    explicit AM2302Configuration(GpioResource dataGpio = GpioResource())
        : gpio(dataGpio) {
    }

    GpioResource gpio;
};

struct SensorImplementationConfiguration {
    SimulatedTemperatureConfiguration simulatedTemperature;
    SimulatedHumidityConfiguration simulatedHumidity;
    SimulatedPressureConfiguration simulatedPressure;
    AM2302Configuration am2302;
};

struct SensorSlotConfiguration {
    SensorId slotId = InvalidSensorId;
    bool enabled = false;
    const char* name = "";
    SensorImplementation implementation = SensorImplementation::Unknown;
    SensorSchedule schedule;
    HardwareResourceAssignment hardware;
    SensorImplementationConfiguration implementationConfiguration;
};

} // namespace WeatherStation
