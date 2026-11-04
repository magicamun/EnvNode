// EnvNode-owned: see repository LICENSE.
#pragma once
#include "Configuration.h"
#include "ILogTimeProvider.h"
#include "LogEntry.h"
#include "RecentLogStore.h"
namespace EnvNode {
void beginNetworkLogging(const Configuration&,ILogTimeProvider&,const RecentLogStore&);
void serviceNetworkLogging(const Configuration&,uint32_t applicationUs=0);
void enqueueNetworkSystemLog(const LogEntry&);
bool networkLoggingAvailable();
bool extendedDiagnosticsAvailable();
String networkLoggingStatus();
String extendedDiagnosticsStatus();
}
