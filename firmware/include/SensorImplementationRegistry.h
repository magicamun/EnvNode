#pragma once

#include <cstddef>

#include "HardwareResources.h"
#include "MeasurementType.h"
#include "SensorProvenance.h"
#include "SensorSchedule.h"

namespace EnvNode {

enum class SensorImplementation : uint8_t {
    None = 0,
    SimulatedTemperature = 1,
    SimulatedHumidity = 2,
    SimulatedPressure = 3,
    AM2302 = 4,
    RainGauge = 5,
    BME280 = 6,
    SHT4x = 7,
};

constexpr size_t MaxImplementationMeasurementTypeCount = 3;

struct SensorImplementationMetadata {
    SensorImplementation implementation;
    const char* stableId;
    const char* displayType;
    SensorProvenance provenance;
    MeasurementType measurementTypes[MaxImplementationMeasurementTypeCount];
    size_t measurementTypeCount;
    HardwareInterfaceKind interfaceKind;
    GpioCapability requiredGpioCapabilities;
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

} // namespace EnvNode
