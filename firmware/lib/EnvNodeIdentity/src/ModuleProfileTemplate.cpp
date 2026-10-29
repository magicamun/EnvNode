// EnvNode: see repository LICENSE.
#include "ModuleProfileTemplate.h"
#include "HardwareDescriptorCodec.h"
#include "CompactCborCodec.h"
#include <memory>
#include <new>
#include <cstring>
namespace EnvNode {
namespace {
bool copied(CompactCborReader& r,CompactCborWriter& w,const uint8_t* bytes){
    const size_t start=r.consumed();
    return r.skip()&&w.writeEncodedValue(bytes+start,r.consumed()-start);
}
void nullable(CompactCborWriter& w,const char* text){if(text&&*text)w.writeText(text);else w.writeNull();}
bool bounded(const char* text,size_t max){return !text||strlen(text)<=max;}
}
bool instantiateModuleProfile(const uint8_t* bytes,size_t size,
    const ModuleManufacturingData& m,uint8_t* output,size_t capacity,size_t& outputSize){
    outputSize=0;
    if(!bounded(m.serialNumber,64)||!bounded(m.productionBatch,64)||!bounded(m.productionDate,10))return false;
    std::unique_ptr<HardwareDescriptor> validation(new(std::nothrow) HardwareDescriptor);
    if(!validation||HardwareDescriptorCodec::decode(bytes,size,HardwareDescriptorObjectKind::Module,*validation)!=HardwareDescriptorDecodeStatus::Valid)return false;
    for(uint8_t byte:validation->instanceId)if(byte)return false;
    validation.reset();
    CompactCborReader r(bytes,size);CompactCborWriter w(output,capacity);size_t pairs=0;
    if(!r.enterMap(pairs)||!w.beginMap(pairs))return false;
    for(size_t i=0;i<pairs;++i){uint64_t key=0;if(!r.readUnsigned(key)||!w.writeUnsigned(key))return false;
        if(key==2){size_t identity=0;if(!r.enterMap(identity)||!w.beginMap(identity))return false;
            for(size_t j=0;j<identity;++j){uint64_t field=0;if(!r.readUnsigned(field)||!w.writeUnsigned(field))return false;
                if(field==9){if(!r.skip()||!w.writeByteString(m.instanceId,16))return false;}
                else if(!copied(r,w,bytes))return false;
            }
        }else if(key==6){if(!r.skip()||!w.beginMap(3))return false;
            w.writeUnsigned(40);nullable(w,m.serialNumber);w.writeUnsigned(41);nullable(w,m.productionBatch);w.writeUnsigned(42);nullable(w,m.productionDate);
        }else if(!copied(r,w,bytes))return false;
    }
    if(r.remaining()||w.status()!=CompactCborStatus::Success)return false;
    outputSize=w.size();
    // Validate generated output with exactly the same generic decoder used before writing.
    validation.reset(new(std::nothrow) HardwareDescriptor);
    return validation&&HardwareDescriptorCodec::decode(output,outputSize,HardwareDescriptorObjectKind::Module,*validation)==HardwareDescriptorDecodeStatus::Valid;
}
}
