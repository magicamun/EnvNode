// EnvNode: see repository LICENSE.
#include "SdModuleActivation.h"
#include <cstring>
namespace EnvNode {
void applyDiscoveredSdModule(const ModuleDiscoveryService& modules,SdStorageConfiguration& settings) {
    unsigned found=0;bool supported=false;ModuleSlot selected=ModuleSlot::A;
    for(size_t i=0;i<ModuleDiscoveryService::SlotCount;++i) {
        const auto* m=modules.result(static_cast<ModuleSlot>(i));
        if(!m || !m->identified() || !m->descriptorTypeId.equals("org.envnode.module.sd-card")) continue;
        ++found;selected=m->slot;
        supported=m->descriptorCompatibility==HardwareDescriptorCompatibilityStatus::Compatible
            && sdModuleContractSupported(m->descriptor);
    }
    settings.automatic=found!=0;settings.moduleConflict=found>1 || (found==1 && !supported);
    if(found) {settings.enabled=found==1 && supported;settings.slot=selected;}
    else if(settings.enabled) {
        const auto& board=currentBoardProfile();const auto* slot=boardModuleSlot(board,settings.slot);
        if(!slot) {settings.enabled=false;settings.moduleConflict=true;return;}
        const uint8_t pins[]={board.spiClock.number,board.spiMiso.number,board.spiMosi.number,slot->spiChipSelect.number,slot->auxGpio1.number};
        for(uint8_t pin:pins) if(modules.ownsHardware(HardwareResourceAssignment::gpioResource(GpioResource(pin)))) {
            settings.enabled=false;settings.moduleConflict=true;break;
        }
    }
}
}
