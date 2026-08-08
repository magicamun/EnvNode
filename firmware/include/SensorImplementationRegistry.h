#pragma once

#include <cstddef>

#include "HardwareResources.h"
#include "MeasurementType.h"
#include "SensorProvenance.h"
#include "SensorSchedule.h"

namespace WeatherStation {

enum class SensorImplementation : uint8_t {
    None = 0,
    SimulatedTemperature = 1,
    SimulatedHumidity = 2,
    SimulatedPressure = 3,
    AM2302 = 4,
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
    static const SensorImplementationMetadata* findByStableId(const char* stableId);
};

} // namespace WeatherStation
