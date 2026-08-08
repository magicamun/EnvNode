#pragma once

#include <cstddef>

#include "HardwareResources.h"
#include "MeasurementType.h"
#include "SensorProvenance.h"
#include "SensorSchedule.h"

namespace WeatherStation {

enum class SensorImplementation {
    Unknown,
    SimulatedTemperature,
    SimulatedHumidity,
    SimulatedPressure,
    AM2302,
};

constexpr size_t MaxImplementationMeasurementTypeCount = 2;

struct SensorImplementationMetadata {
    SensorImplementation implementation;
    const char* stableId;
    const char* displayType;
    SensorProvenance provenance;
    MeasurementType measurementTypes[MaxImplementationMeasurementTypeCount];
    size_t measurementTypeCount;
    HardwareInterfaceKind interfaceKind;
    const char* protocolDescription;
    SensorSchedule defaultSchedule;
    const char* configurationSchemaDescription;
};

class SensorImplementationRegistry {
public:
    static size_t count();
    static const SensorImplementationMetadata* at(size_t index);
    static const SensorImplementationMetadata* find(SensorImplementation implementation);
};

} // namespace WeatherStation
