// EnvNode: see repository LICENSE.
#include "SdModuleActivation.h"
#include <cstring>
namespace EnvNode {
namespace {
bool binding(const HardwareDescriptor& d,const DescriptorDevice& device,const char* name,
    const char* resource,HardwareDescriptorResourceKind kind) {
    for(size_t i=0;i<device.bindingCount;++i) if(device.bindings[i].name.equals(name)) {
        for(size_t j=0;j<d.requirementCount;++j) {
            const auto& r=d.requirements[j];
            if(r.id.size==device.bindings[i].target.size && r.id.data && device.bindings[i].target.data
                && !memcmp(r.id.data,device.bindings[i].target.data,r.id.size))
                return r.resource.equals(resource) && r.kind==kind;
        }
        return false;
    }
    return false;
}
}
bool sdModuleContractSupported(const HardwareDescriptor& d) {
    if(d.deviceCount!=1) return false;
    const auto& device=d.devices[0];
    return device.kind==HardwareDescriptorDeviceKind::Infrastructure
        && !device.driver.id.coded && device.driver.id.text.equals("org.envnode.driver.sd-spi")
        && device.driver.apiVersion==1 && device.hasActiveLevel && device.activeLevelHigh
        && device.capabilities.contains(HardwareDescriptorCapabilityCode::StorageSd)
        && binding(d,device,"bus","SPI",HardwareDescriptorResourceKind::SPI)
        && binding(d,device,"chipSelect","SPI_CS",HardwareDescriptorResourceKind::Gpio)
        && binding(d,device,"cardDetect","AUX_GPIO1",HardwareDescriptorResourceKind::Gpio);
}
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
