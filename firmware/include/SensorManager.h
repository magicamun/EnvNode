#pragma once

#include <cstddef>
#include <cstdint>

#include "IMeasurementSink.h"
#include "IMeasurementObserver.h"
#include "IMonotonicClock.h"
#include "ISensor.h"
#include "ITimeService.h"
#include "SensorSchedule.h"
#include "SensorImplementationRegistry.h"
#include "SensorSlotConfiguration.h"

namespace EnvNode {

constexpr size_t MaxSensorCount = 16;

enum class SensorRegistrationResult {
    Registered,
    ManagerAlreadyStarted,
    InvalidSensorId,
    DuplicateSensorId,
    CapacityReached,
    InvalidSchedule,
};

struct SensorRuntimeStatus {
    bool serviceObserved = false;
    SensorOperationResult lastServiceResult = SensorOperationResult::NoData;
    uint32_t lastServiceEmissionCount = 0;
    bool sampleObserved = false;
    SensorOperationResult lastSampleResult = SensorOperationResult::NoData;
    uint32_t lastSampleEmissionCount = 0;
    uint32_t acceptedMeasurementCount = 0;
    uint32_t rejectedMeasurementCount = 0;
    uint32_t preSyncDiscardCount = 0;
    bool hasMeasurementActivity = false;
    uint32_t lastMeasurementMonotonicMs = 0;
    bool hasLastMeasurement = false;
    time_t lastMeasurementEpoch = 0;
};

struct SensorRuntimeInfo {
    SensorId id = InvalidSensorId;
    const char* name = "";
    const char* type = "";
    SensorImplementation implementation = SensorImplementation::None;
    const char* interfaceName = "";
    const char* protocolDescription = "";
    const char* configurationSummary = "";
    HardwareResourceAssignment hardware;
    SensorProvenance provenance = SensorProvenance::Physical;
    SensorState state = SensorState::Unknown;
    SensorSchedule schedule;
    bool supportsTemperature = false;
    bool supportsRelativeHumidity = false;
    bool supportsAtmosphericPressure = false;
    bool supportsSolarIrradiance = false;
    bool supportsSolarCellTemperature = false;
    bool supportsRainDetectorLevel = false;
    bool supportsRainDetectorWet = false;
    bool supportsRainGaugeTip = false;
    bool supportsRainfallIncrement = false;
    bool supportsHydrostaticPressure = false;
    bool supportsWaterLevel = false;
};

struct SensorRegistrationMetadata {
    SensorRegistrationMetadata(
        const char* sensorName = "",
        SensorImplementation sensorImplementation = SensorImplementation::None,
        const char* sensorInterfaceName = "",
        const char* sensorProtocolDescription = "",
        const char* sensorConfigurationSummary = "",
        HardwareResourceAssignment sensorHardware = HardwareResourceAssignment())
        : name(sensorName)
        , implementation(sensorImplementation)
        , interfaceName(sensorInterfaceName)
        , protocolDescription(sensorProtocolDescription)
        , configurationSummary(sensorConfigurationSummary)
        , hardware(sensorHardware) {
    }

    const char* name;
    SensorImplementation implementation;
    const char* interfaceName;
    const char* protocolDescription;
    const char* configurationSummary;
    HardwareResourceAssignment hardware;
};

class SensorManager : public IMeasurementSink {
public:
    SensorManager(
        ITimeService& timeService,
        IMonotonicClock& monotonicClock,
        IMeasurementSink& downstream,
        IMeasurementObserver& measurementObserver);

    SensorRegistrationResult registerSensor(ISensor& sensor, const SensorSchedule& schedule);
    SensorRegistrationResult registerSensor(
        ISensor& sensor,
        const SensorSchedule& schedule,
        const SensorRegistrationMetadata& metadata);
    void begin();
    void clear();
    void loop();

    size_t sensorCount() const;
    bool runtimeStatus(SensorId id, SensorRuntimeStatus& status) const;
    bool runtimeInfo(size_t index, SensorRuntimeInfo& info) const;

    void emit(const Measurement& measurementContent) override;

private:
    struct SensorEntry {
        ISensor* sensor = nullptr;
        SensorId registeredId = InvalidSensorId;
        char name[MaxSensorSlotNameLength + 1] = {};
        SensorImplementation implementation = SensorImplementation::None;
        const char* interfaceName = "";
        const char* protocolDescription = "";
        const char* configurationSummary = "";
        HardwareResourceAssignment hardware;
        SensorSchedule schedule;
        uint32_t nextDueMs = 0;
        bool samplePending = false;
        bool intervalMissedWhilePending = false;
        SensorRuntimeStatus status;
    };

    enum class OperationKind {
        Service,
        Sample,
    };

    static bool isScheduleValid(const SensorSchedule& schedule);
    static bool deadlineReached(uint32_t nowMs, uint32_t deadlineMs);

    void runOperation(SensorEntry& entry, OperationKind kind);
    void observeResult(SensorEntry& entry, OperationKind kind, SensorOperationResult result);
    void advanceDeadline(SensorEntry& entry, bool intervalMissedWhilePending);
    SensorEntry* findEntry(SensorId id);
    const SensorEntry* findEntry(SensorId id) const;

    ITimeService& timeService_;
    IMonotonicClock& monotonicClock_;
    IMeasurementSink& downstream_;
    IMeasurementObserver& measurementObserver_;
    SensorEntry entries_[MaxSensorCount];
    size_t sensorCount_ = 0;
    bool started_ = false;

    SensorEntry* activeEntry_ = nullptr;
    time_t operationTimestamp_ = 0;
    uint32_t operationMonotonicMs_ = 0;
    bool operationTimeSynchronized_ = false;
    uint32_t operationEmissionCount_ = 0;
};

} // namespace EnvNode
