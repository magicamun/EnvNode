#pragma once

#include <Arduino.h>

#include "HardwareResources.h"
#include "SensorId.h"
#include "SensorImplementationRegistry.h"
#include "SensorSchedule.h"

namespace EnvNode {

struct SimulatedTemperatureConfiguration {};
struct SimulatedHumidityConfiguration {};
struct SimulatedPressureConfiguration {};

struct AM2302Configuration {
    explicit AM2302Configuration(GpioResource dataGpio = GpioResource())
        : gpio(dataGpio) {
    }

    GpioResource gpio;
};

struct BME280Configuration {
    explicit BME280Configuration(I2CResource i2cResource = I2CResource())
        : i2c(i2cResource) {
    }

    I2CResource i2c;
};

struct SHT4xConfiguration {
    explicit SHT4xConfiguration(I2CResource i2cResource = I2CResource(I2CBus::I2C0, 0x44))
        : i2c(i2cResource) {
    }

    I2CResource i2c;
};

struct RainGaugeConfiguration {
    RainGaugeConfiguration(
        GpioResource inputGpio = GpioResource(),
        float rainfallMillimetersPerTip = 0.2794F,
        uint32_t softwareDebounceMs = 50)
        : gpio(inputGpio)
        , millimetersPerTip(rainfallMillimetersPerTip)
        , debounceMs(softwareDebounceMs) {
    }

    GpioResource gpio;
    float millimetersPerTip;
    uint32_t debounceMs;
};

struct SensorImplementationConfiguration {
    SimulatedTemperatureConfiguration simulatedTemperature;
    SimulatedHumidityConfiguration simulatedHumidity;
    SimulatedPressureConfiguration simulatedPressure;
    AM2302Configuration am2302;
    BME280Configuration bme280;
    SHT4xConfiguration sht4x;
    RainGaugeConfiguration rainGauge;
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

} // namespace EnvNode
