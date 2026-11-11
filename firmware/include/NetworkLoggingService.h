// EnvNode-owned: see repository LICENSE.
#pragma once
#include "Configuration.h"
#include "ILogTimeProvider.h"
#include "LogEntry.h"
#include "RecentLogStore.h"
#include "TestMarkerSession.h"
namespace EnvNode {
void beginNetworkLogging(const Configuration&,ILogTimeProvider&,const RecentLogStore&);
void serviceNetworkLogging(const Configuration&,uint32_t applicationUs=0);
void enqueueNetworkSystemLog(const LogEntry&);
bool networkLoggingAvailable();
bool extendedDiagnosticsAvailable();
String networkLoggingStatus();
String extendedDiagnosticsStatus();
const TestMarkerSession& networkTestMarkers();
const char* networkLoggingBootId();
TestMarkerResult recordNetworkTestMarker(const Configuration&,TestMarkerAction,
    const char* label,const char* note,uint32_t expectedActiveId);
}
