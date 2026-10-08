#pragma once

#include <Arduino.h>
#include "MeasurementType.h"
#include "DisplayConfiguration.h"
#include "ValueConfiguration.h"
#include "EnvNode/Locale.h"
#include "SensorSlotConfiguration.h"
#include "ActuatorSlotConfiguration.h"
#include "ControllerSlotConfiguration.h"

namespace EnvNode {

enum class NetworkAddressMode : uint8_t {
    Dhcp = 0,
    Static = 1,
};

struct DeviceConfiguration {
    String name;
};

struct NetworkConfiguration {
    String hostname;
    String wifiSSID;
    String wifiPassword;
    NetworkAddressMode addressMode = NetworkAddressMode::Dhcp;
    String ipv4Address;
    String subnetMask;
    String gateway;
    String dns1;
    String dns2;
};

struct MqttConfiguration {
    String server;
    uint16_t port = 1883;
    String username;
    String password;
};

struct TimeConfiguration {
    String timezone;
    String ntpServer1;
    String ntpServer2;
};

struct LocaleConfiguration {
    Locale locale = Locale::GermanGermany;
};

struct PresentationConfiguration {
    PresentationUnit temperature = PresentationUnit::DegreeCelsius;
    PresentationUnit atmosphericPressure = PresentationUnit::Pascal;
    PresentationUnit solarCellTemperature = PresentationUnit::DegreeCelsius;
    PresentationUnit rainDetectorLevel = PresentationUnit::Ratio;

    PresentationUnit unitFor(MeasurementType type) const {
        switch (type) {
            case MeasurementType::Temperature: return temperature;
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
