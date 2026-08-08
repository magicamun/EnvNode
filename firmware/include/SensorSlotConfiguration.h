#pragma once

#include <Arduino.h>

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
    String name;
    SensorImplementation implementation = SensorImplementation::None;
    SensorSchedule schedule;
    HardwareResourceAssignment hardware;
    SensorImplementationConfiguration implementationConfiguration;
};

constexpr size_t MaxSensorSlotCount = 16;
constexpr size_t MaxSensorSlotNameLength = 32;

} // namespace WeatherStation
