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
#include "SdSystemLog.h"
#include "SdRotationTest.h"
#include <memory>
namespace EnvNode {
bool sdStoragePinsAvailable(const Configuration& configuration,const ActuatorRuntime& actuators);
std::unique_ptr<ISdDirectoryJob> startSdDirectoryRead(ModuleSlot slot,const BoardProfile& board,
    const Configuration& configuration,const ActuatorRuntime& actuators,
    const String& path,unsigned offset,SdDirectoryStatus& status);
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
std::unique_ptr<ISdRotationTestJob> startSdRotationTest(ModuleSlot slot,const BoardProfile& board,
    const Configuration& configuration,const ActuatorRuntime& actuators,SdDirectoryStatus& status);
std::unique_ptr<ISdLogStorage> createSdSystemLogStorage(ModuleSlot slot,uint32_t session);
// Explicitly requested write/read test; no automatic startup writes.
void runSdCardSpiDiagnostic(ModuleSlot slot, const BoardProfile& board,
    const Configuration& configuration, const AutomaticActuatorDefinition* automaticActuators,
    size_t automaticCount, ILogger& logger);
}
