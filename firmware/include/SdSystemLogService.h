// EnvNode: see repository LICENSE.
#pragma once
#include "Configuration.h"
#include "ActuatorRuntime.h"
#include "RecentLogStore.h"
#include "Logger.h"
#include "ILogTimeProvider.h"
namespace EnvNode {
void ejectSdCardRecording();
void enqueueSdSystemLog(const LogEntry& entry);
void beginSdSystemLog(const Configuration& configuration,const ActuatorRuntime& actuators,const RecentLogStore& recent,ILogger& logger,ILogTimeProvider& clock);
void serviceSdSystemLog(const Configuration& configuration,const ActuatorRuntime& actuators,ILogger& logger);
void configureSdSystemLog(const Configuration& configuration,const ActuatorRuntime& actuators);
bool sdSystemLogSlotEnabled(ModuleSlot slot);
bool sdDiagnosticSlotEnabled(ModuleSlot slot);
bool extendedDiagnosticsAvailable();
String extendedDiagnosticsStatus();
String sdSystemLogStatus();
}
