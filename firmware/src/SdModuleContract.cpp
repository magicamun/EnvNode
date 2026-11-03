// EnvNode: see repository LICENSE.
#include "SdModuleContract.h"
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
}
