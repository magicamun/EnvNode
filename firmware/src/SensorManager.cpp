#include "SensorManager.h"

#include <cstring>

namespace WeatherStation {
namespace {

const uint32_t MaximumScheduleIntervalMs = 0x7FFFFFFFUL;

} // namespace

SensorManager::SensorManager(
    ITimeService& timeService,
    IMonotonicClock& monotonicClock,
    IMeasurementSink& downstream)
    : timeService_(timeService)
    , monotonicClock_(monotonicClock)
    , downstream_(downstream) {
}

SensorRegistrationResult SensorManager::registerSensor(
    ISensor& sensor,
    const SensorSchedule& schedule) {
    return registerSensor(sensor, schedule, SensorRegistrationMetadata{});
}

SensorRegistrationResult SensorManager::registerSensor(
    ISensor& sensor,
    const SensorSchedule& schedule,
    const SensorRegistrationMetadata& metadata) {
    if (started_) {
        return SensorRegistrationResult::ManagerAlreadyStarted;
    }

    if (!isValidSensorId(sensor.id())) {
        return SensorRegistrationResult::InvalidSensorId;
    }

    if (findEntry(sensor.id()) != nullptr) {
        return SensorRegistrationResult::DuplicateSensorId;
    }

    if (sensorCount_ >= MaxSensorCount) {
        return SensorRegistrationResult::CapacityReached;
    }

    if (!isScheduleValid(schedule)) {
        return SensorRegistrationResult::InvalidSchedule;
    }

    SensorEntry& entry = entries_[sensorCount_++];
    entry.sensor = &sensor;
    entry.registeredId = sensor.id();
    strncpy(entry.name, metadata.name == nullptr ? "" : metadata.name, MaxSensorSlotNameLength);
    entry.name[MaxSensorSlotNameLength] = '\0';
    entry.implementation = metadata.implementation;
    entry.interfaceName = metadata.interfaceName == nullptr ? "" : metadata.interfaceName;
    entry.protocolDescription = metadata.protocolDescription == nullptr ? "" : metadata.protocolDescription;
    entry.configurationSummary = metadata.configurationSummary == nullptr ? "" : metadata.configurationSummary;
    entry.hardware = metadata.hardware;
    entry.schedule = schedule;
    return SensorRegistrationResult::Registered;
}

void SensorManager::begin() {
    if (started_) {
        return;
    }

    started_ = true;
    const uint32_t nowMs = monotonicClock_.nowMs();

    for (size_t index = 0; index < sensorCount_; ++index) {
        SensorEntry& entry = entries_[index];
        entry.nextDueMs = nowMs;
        entry.samplePending = false;
        entry.intervalMissedWhilePending = false;

        if (entry.schedule.enabled) {
            entry.sensor->begin();
        }
    }
}

void SensorManager::clear() {
    activeEntry_ = nullptr;
    started_ = false;
    sensorCount_ = 0;
    operationTimestamp_ = 0;
    operationMonotonicMs_ = 0;
    operationTimeSynchronized_ = false;
    operationEmissionCount_ = 0;
    for (size_t index = 0; index < MaxSensorCount; ++index) {
        entries_[index] = SensorEntry{};
    }
}

void SensorManager::loop() {
    if (!started_) {
        return;
    }

    for (size_t index = 0; index < sensorCount_; ++index) {
        SensorEntry& entry = entries_[index];

        if (!entry.schedule.enabled) {
            continue;
        }

        runOperation(entry, OperationKind::Service);

        if (entry.schedule.acquisitionMode != AcquisitionMode::Periodic) {
            continue;
        }

        const uint32_t nowMs = monotonicClock_.nowMs();
        if (!entry.samplePending && deadlineReached(nowMs, entry.nextDueMs)) {
            entry.samplePending = true;
            entry.intervalMissedWhilePending = false;
        } else if (entry.samplePending
            && !entry.intervalMissedWhilePending
            && deadlineReached(nowMs, entry.nextDueMs + entry.schedule.sampleIntervalMs)) {
            entry.intervalMissedWhilePending = true;
        }

        const SensorState state = entry.sensor->state();
        if (state != SensorState::Ready && state != SensorState::Degraded) {
            continue;
        }

        if (!entry.samplePending) {
            continue;
        }

        runOperation(entry, OperationKind::Sample);
        const bool intervalMissedWhilePending = entry.intervalMissedWhilePending;
        entry.samplePending = false;
        entry.intervalMissedWhilePending = false;
        advanceDeadline(entry, intervalMissedWhilePending);
    }
}

size_t SensorManager::sensorCount() const {
    return sensorCount_;
}

bool SensorManager::runtimeStatus(SensorId id, SensorRuntimeStatus& status) const {
    const SensorEntry* entry = findEntry(id);
    if (entry == nullptr) {
        return false;
    }

    status = entry->status;
    return true;
}

bool SensorManager::runtimeInfo(size_t index, SensorRuntimeInfo& info) const {
    if (index >= sensorCount_) return false;
    const SensorEntry& entry = entries_[index];
    info.id = entry.registeredId;
    info.name = entry.name;
    const SensorImplementationMetadata* implementation =
        SensorImplementationRegistry::find(entry.implementation);
    info.type = implementation == nullptr ? entry.sensor->type() : implementation->displayType;
    info.implementation = entry.implementation;
    info.interfaceName = entry.interfaceName;
    info.protocolDescription = entry.protocolDescription;
    info.configurationSummary = entry.configurationSummary;
    info.hardware = entry.hardware;
    info.provenance = entry.sensor->provenance();
    info.state = entry.sensor->state();
    info.schedule = entry.schedule;
    info.supportsTemperature = entry.sensor->supports(MeasurementType::Temperature);
    info.supportsRelativeHumidity = entry.sensor->supports(MeasurementType::RelativeHumidity);
    info.supportsAtmosphericPressure = entry.sensor->supports(MeasurementType::AtmosphericPressure);
    info.supportsSolarIrradiance = entry.sensor->supports(MeasurementType::SolarIrradiance);
    info.supportsSolarCellTemperature = entry.sensor->supports(MeasurementType::SolarCellTemperature);
    info.supportsRainDetectorLevel = entry.sensor->supports(MeasurementType::RainDetectorLevel);
    info.supportsRainDetectorWet = entry.sensor->supports(MeasurementType::RainDetectorWet);
    info.supportsRainGaugeTip = entry.sensor->supports(MeasurementType::RainGaugeTip);
    info.supportsRainfallIncrement = entry.sensor->supports(MeasurementType::RainfallIncrement);
    return true;
}

void SensorManager::emit(const Measurement& measurementContent) {
    if (activeEntry_ == nullptr) {
        return;
    }

    ++operationEmissionCount_;

    if (!activeEntry_->sensor->supports(measurementContent.type)
        || !isMeasurementContentStructurallyValid(measurementContent)) {
        ++activeEntry_->status.rejectedMeasurementCount;
        return;
    }

    activeEntry_->status.hasMeasurementActivity = true;
    activeEntry_->status.lastMeasurementMonotonicMs = operationMonotonicMs_;

    if (!operationTimeSynchronized_) {
        ++activeEntry_->status.preSyncDiscardCount;
        return;
    }

    Measurement completedMeasurement = measurementContent;
    completedMeasurement.source = activeEntry_->sensor->id();
    completedMeasurement.timestamp = operationTimestamp_;
    completedMeasurement.provenance = activeEntry_->sensor->provenance();

    if (!isMeasurementStructurallyValid(completedMeasurement)) {
        ++activeEntry_->status.rejectedMeasurementCount;
        return;
    }

    activeEntry_->status.hasLastMeasurement = true;
    activeEntry_->status.lastMeasurementEpoch = completedMeasurement.timestamp;
    downstream_.emit(completedMeasurement);
    ++activeEntry_->status.acceptedMeasurementCount;
}

bool SensorManager::isScheduleValid(const SensorSchedule& schedule) {
    switch (schedule.acquisitionMode) {
        case AcquisitionMode::EventOnly:
            return schedule.sampleIntervalMs == 0;
        case AcquisitionMode::Periodic:
            return schedule.sampleIntervalMs > 0
                && schedule.sampleIntervalMs <= MaximumScheduleIntervalMs;
        default:
            return false;
    }
}

bool SensorManager::deadlineReached(uint32_t nowMs, uint32_t deadlineMs) {
    return static_cast<int32_t>(nowMs - deadlineMs) >= 0;
}

void SensorManager::runOperation(SensorEntry& entry, OperationKind kind) {
    activeEntry_ = &entry;
    operationEmissionCount_ = 0;
    operationMonotonicMs_ = monotonicClock_.nowMs();
    operationTimeSynchronized_ = timeService_.synchronized();
    operationTimestamp_ = operationTimeSynchronized_ ? timeService_.now() : 0;

    const SensorOperationResult result = kind == OperationKind::Service
        ? entry.sensor->service(*this)
        : entry.sensor->sample(*this);

    activeEntry_ = nullptr;
    operationTimestamp_ = 0;
    operationMonotonicMs_ = 0;
    operationTimeSynchronized_ = false;

    observeResult(entry, kind, result);
}

void SensorManager::observeResult(
    SensorEntry& entry,
    OperationKind kind,
    SensorOperationResult result) {
    if (kind == OperationKind::Service) {
        entry.status.serviceObserved = true;
        entry.status.lastServiceResult = result;
        entry.status.lastServiceEmissionCount = operationEmissionCount_;
        return;
    }

    entry.status.sampleObserved = true;
    entry.status.lastSampleResult = result;
    entry.status.lastSampleEmissionCount = operationEmissionCount_;
}

void SensorManager::advanceDeadline(
    SensorEntry& entry,
    bool intervalMissedWhilePending) {
    if (intervalMissedWhilePending) {
        entry.nextDueMs = monotonicClock_.nowMs() + entry.schedule.sampleIntervalMs;
        return;
    }

    entry.nextDueMs += entry.schedule.sampleIntervalMs;

    const uint32_t nowMs = monotonicClock_.nowMs();
    if (deadlineReached(nowMs, entry.nextDueMs)) {
        entry.nextDueMs = nowMs + entry.schedule.sampleIntervalMs;
    }
}

SensorManager::SensorEntry* SensorManager::findEntry(SensorId id) {
    for (size_t index = 0; index < sensorCount_; ++index) {
        if (entries_[index].registeredId == id) {
            return &entries_[index];
        }
    }

    return nullptr;
}

const SensorManager::SensorEntry* SensorManager::findEntry(SensorId id) const {
    for (size_t index = 0; index < sensorCount_; ++index) {
        if (entries_[index].registeredId == id) {
            return &entries_[index];
        }
    }

    return nullptr;
}

} // namespace WeatherStation
