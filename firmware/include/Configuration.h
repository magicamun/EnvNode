#pragma once

#include <Arduino.h>
#include "MeasurementType.h"

namespace WeatherStation {

struct Configuration {
    String deviceName;
    String wifiSSID;
    String wifiPassword;
    String mqttServer;
    uint16_t mqttPort;
    String mqttUsername;
    String mqttPassword;
    String timezone;
    String ntpServer1;
    String ntpServer2;
    PresentationUnit temperaturePresentationUnit = PresentationUnit::DegreeCelsius;
    PresentationUnit atmosphericPressurePresentationUnit = PresentationUnit::Pascal;
    PresentationUnit solarCellTemperaturePresentationUnit = PresentationUnit::DegreeCelsius;
    PresentationUnit rainDetectorLevelPresentationUnit = PresentationUnit::Ratio;

    PresentationUnit presentationUnitFor(MeasurementType type) const {
        switch (type) {
            case MeasurementType::Temperature: return temperaturePresentationUnit;
            case MeasurementType::AtmosphericPressure: return atmosphericPressurePresentationUnit;
            case MeasurementType::SolarCellTemperature: return solarCellTemperaturePresentationUnit;
            case MeasurementType::RainDetectorLevel: return rainDetectorLevelPresentationUnit;
            default: return measurementTypeMetadata(type).defaultPresentationUnit;
        }
    }
};

} // namespace WeatherStation
