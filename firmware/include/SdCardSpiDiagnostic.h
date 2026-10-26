// EnvNode: see repository LICENSE.
#pragma once
#include <cstddef>
#include "Configuration.h"
#include "ActuatorRuntime.h"
#include "BoardProfile.h"
#include "Logger.h"
#include "SdCardWebView.h"
#include "SdFileTransfer.h"
#include "SdCardFormat.h"
#include <memory>
namespace EnvNode {
void readSdCardDirectory(ModuleSlot slot, const BoardProfile& board,
    const Configuration& configuration, const ActuatorRuntime& actuators, SdDirectorySnapshot& snapshot);
std::unique_ptr<ISdStoredFile> openSdCardFile(ModuleSlot slot, const BoardProfile& board,
    const Configuration& configuration, const ActuatorRuntime& actuators,
    const String& path, SdDirectoryStatus& status);
bool inspectSdCardForFormat(ModuleSlot slot,const BoardProfile& board,
    const Configuration& configuration,const ActuatorRuntime& actuators,
    SdFileStamp& card,SdDirectoryStatus& status);
std::unique_ptr<ISdFormatJob> startSdCardFormat(ModuleSlot slot,const BoardProfile& board,
    const Configuration& configuration,const ActuatorRuntime& actuators,
    const SdFileStamp& expected,SdDirectoryStatus& status);
// Explicit lab build only; no automatic SD module detection yet.
void runSdCardSpiDiagnostic(ModuleSlot slot, const BoardProfile& board,
    const Configuration& configuration, const AutomaticActuatorDefinition* automaticActuators,
    size_t automaticCount, ILogger& logger);
}
