// EnvNode: see repository LICENSE.
#pragma once
#include "Configuration.h"
#include "ActuatorRuntime.h"
#include "BoardProfile.h"
namespace EnvNode {
// Hardware eligibility must not depend on the mount state or the storage lease.
inline const BoardModuleSlotCapability* configuredSdStorageSlot(ModuleSlot slotId,const BoardProfile& board,
    const Configuration& configuration,const ActuatorRuntime& actuators) {
    if(!configuration.sdStorage.enabled || configuration.sdStorage.moduleConflict
        || configuration.sdStorage.slot!=slotId)return nullptr;
    const auto* slot=boardModuleSlot(board,slotId);
    if(!slot || !slot->fiveVoltSupplyAvailable)return nullptr;
    const auto conflicts=[&](const HardwareResourceAssignment& assignment) {
        if(assignment.kind!=HardwareResourceKind::GPIO)return false;
        const uint8_t pin=assignment.gpio.number;
        return pin==board.spiClock.number || pin==board.spiMiso.number || pin==board.spiMosi.number
            || pin==slot->spiChipSelect.number || pin==slot->auxGpio1.number;
    };
    for(const auto& sensor:configuration.sensorSlots)if(sensor.enabled && conflicts(sensor.hardware))return nullptr;
    for(const auto& actor:configuration.actuatorSlots)if(actor.enabled && conflicts(actor.hardware))return nullptr;
    for(size_t i=0;i<actuators.runtimeCount();++i) {
        ActuatorRuntimeInfo info;
        if(actuators.runtimeInfo(i,info) && conflicts(info.hardware))return nullptr;
    }
    return slot;
}
inline bool sdStoragePinsAvailable(const Configuration& configuration,const ActuatorRuntime& actuators) {
    return configuredSdStorageSlot(configuration.sdStorage.slot,currentBoardProfile(),configuration,actuators)!=nullptr;
}

}
