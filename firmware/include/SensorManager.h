#pragma once

#include <cstddef>
#include <cstdint>

#include "IMeasurementSink.h"
#include "IMonotonicClock.h"
#include "ISensor.h"
#include "ITimeService.h"
#include "SensorSchedule.h"

namespace WeatherStation {

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
};

struct SensorRuntimeInfo {
    SensorId id = InvalidSensorId;
    const char* name = "";
    const char* type = "";
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
};

struct SensorRegistrationMetadata {
    explicit SensorRegistrationMetadata(const char* sensorName = "")
        : name(sensorName) {
    }

    const char* name;
};

class SensorManager : public IMeasurementSink {
public:
    SensorManager(ITimeService& timeService, IMonotonicClock& monotonicClock, IMeasurementSink& downstream);

    SensorRegistrationResult registerSensor(ISensor& sensor, const SensorSchedule& schedule);
    SensorRegistrationResult registerSensor(
        ISensor& sensor,
        const SensorSchedule& schedule,
        const SensorRegistrationMetadata& metadata);
    void begin();
    void loop();

    size_t sensorCount() const;
    bool runtimeStatus(SensorId id, SensorRuntimeStatus& status) const;
    bool runtimeInfo(size_t index, SensorRuntimeInfo& info) const;

    void emit(const Measurement& measurementContent) override;

private:
    struct SensorEntry {
        ISensor* sensor = nullptr;
        SensorId registeredId = InvalidSensorId;
        const char* name = "";
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
    SensorEntry entries_[MaxSensorCount];
    size_t sensorCount_ = 0;
    bool started_ = false;

    SensorEntry* activeEntry_ = nullptr;
    time_t operationTimestamp_ = 0;
    bool operationTimeSynchronized_ = false;
    uint32_t operationEmissionCount_ = 0;
};

} // namespace WeatherStation
