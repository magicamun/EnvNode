// EnvNode: see repository LICENSE.
#pragma once
#include "Configuration.h"
#include "ActuatorRuntime.h"
#include "RecentLogStore.h"
#include "Logger.h"
namespace EnvNode {
void enqueueSdSystemLog(const LogEntry& entry);
void beginSdSystemLog(const Configuration& configuration,const ActuatorRuntime& actuators,const RecentLogStore& recent,ILogger& logger);
void serviceSdSystemLog(const Configuration& configuration,const ActuatorRuntime& actuators,ILogger& logger);
void configureSdSystemLog(const Configuration& configuration,const ActuatorRuntime& actuators);
bool sdSystemLogSlotEnabled(ModuleSlot slot);
String sdSystemLogStatus();
}
