#pragma once

#include <Arduino.h>
#include "BaseConfiguration.h"
#include "MeasurementType.h"
#include "GeoPosition.h"
#include "PressureReduction.h"
#include "DisplayConfiguration.h"
#include "ValueConfiguration.h"
#include "EnvNode/Locale.h"
#include "SensorSlotConfiguration.h"
#include "ActuatorSlotConfiguration.h"
#include "ControllerSlotConfiguration.h"

namespace EnvNode {

struct MqttConfiguration {
    String server;
    uint16_t port = 1883;
    String username;
    String password;
};

struct PresentationConfiguration {
    PresentationUnit temperature = PresentationUnit::DegreeCelsius;
    PresentationUnit atmosphericPressure = PresentationUnit::Pascal;
    PresentationUnit solarCellTemperature = PresentationUnit::DegreeCelsius;
    PresentationUnit rainDetectorLevel = PresentationUnit::Ratio;

    PresentationUnit unitFor(MeasurementType type) const {
        switch (type) {
            case MeasurementType::Temperature: return temperature;
            case MeasurementType::SeaLevelPressure:
            case MeasurementType::AtmosphericPressure: return atmosphericPressure;
            case MeasurementType::SolarCellTemperature: return solarCellTemperature;
            case MeasurementType::RainDetectorLevel: return rainDetectorLevel;
            default: return measurementTypeMetadata(type).defaultPresentationUnit;
        }
    }
};

struct Configuration {
    ValueConfiguration values;
    DisplayConfiguration display;
    DeviceConfiguration device;
    GeoPosition geoPosition;
    PressureReductionConfiguration pressureReduction;
    NetworkConfiguration network;
    MqttConfiguration mqtt;
    TimeConfiguration time;
    LocaleConfiguration locale;
    PresentationConfiguration presentation;
    SensorSlotConfiguration sensorSlots[MaxSensorSlotCount];
    ActuatorSlotConfiguration actuatorSlots[MaxActuatorSlotCount];
    ControllerSlotConfiguration controllerSlots[MaxControllerSlotCount];

    PresentationUnit presentationUnitFor(MeasurementType type) const {
        return presentation.unitFor(type);
    }
};

} // namespace EnvNode
