// EnvNode: see repository LICENSE.
#include "ProvisioningSnapshot.h"
#include "BoardIdentityStore.h"
#include "BoardIdentityProfiles.h"
#include "BoardPortMap.h"
#include "HardwareDescriptorStore.h"
#include "HardwareDescriptorCodec.h"
#include <memory>
#include <new>
#include <cstring>
#include <cstdio>
namespace EnvNode {
namespace {
constexpr size_t EepromBytes = 4096;
class ReadOnlyImage : public IIdentityStorage {
public:
    explicit ReadOnlyImage(const uint8_t* data):data_(data){}
    bool read(uint16_t offset,uint8_t* out,size_t n) override {
        if(offset+n>EepromBytes)return false;memcpy(out,data_+offset,n);return true;
    }
    bool write(uint16_t,const uint8_t*,size_t) override {return false;}
private: const uint8_t* data_;
};
bool capture(IIdentityStorage& storage,uint8_t* bytes,ProvisioningSnapshot& out,char target) {
    out={};uint64_t hash=UINT64_C(14695981039346656037);
    for(size_t offset=0;offset<EepromBytes;offset+=32){
        if(!storage.read(offset,bytes+offset,32))return false;
        for(size_t i=offset;i<offset+32;++i){hash^=bytes[i];hash*=UINT64_C(1099511628211);}
    }
    snprintf(out.token,sizeof(out.token),"%c:%08lx%08lx",target,static_cast<unsigned long>(hash>>32),static_cast<unsigned long>(hash&0xffffffff));
    return true;
}
template<size_t N> bool copy(char (&out)[N],DescriptorTextView text) {
    if(text.size>=N||(!text.data&&text.size))return false;
    if(text.size)memcpy(out,text.data,text.size);out[text.size]=0;return true;
}
}
bool BoardIdentityStore::snapshot(ProvisioningSnapshot& out) {
    out={};std::unique_ptr<uint8_t[]> bytes(new(std::nothrow) uint8_t[EepromBytes]);
    if(!bytes||!capture(storage_,bytes.get(),out,'b'))return false;
    ReadOnlyImage image(bytes.get());BoardIdentityStore identities(image);BoardPortMapStore maps(image);BoardPortMap map;
    const auto identity=identities.read(false);const auto portmap=maps.read(map);
    out.status=ProvisioningSnapshotStatus::Corrupt;
    if(identity.status==BoardIdentityStatus::NotProvisioned&&portmap.status==BoardPortMapStatus::NotProvisioned){out.status=ProvisioningSnapshotStatus::Empty;return true;}
    if(identity.status!=BoardIdentityStatus::Valid&&identity.status!=BoardIdentityStatus::UnassignedSerial)return true;
    if(portmap.status==BoardPortMapStatus::Valid){
        if(map.profileId!=encodeBoardProfileId(identity.identity.profileId)||map.revision.major!=identity.identity.revision.major||map.revision.minor!=identity.identity.revision.minor||strcmp(map.serial.value,identity.identity.serialNumber.value))return true;
        strcpy(out.name,map.name);strcpy(out.batch,map.productionBatch);strcpy(out.date,map.productionDate);
    }else{
        if(portmap.status!=BoardPortMapStatus::NotProvisioned)return true;
        const auto* profile=boardIdentityProfile(identity.identity.profileId,identity.identity.revision);if(!profile)return true;
        snprintf(out.name,sizeof(out.name),"%s",profile->displayName);
    }
    out.profileId=encodeBoardProfileId(identity.identity.profileId);out.revision=identity.identity.revision;strcpy(out.serial,identity.identity.serialNumber.value);out.status=ProvisioningSnapshotStatus::Valid;return true;
}
bool HardwareDescriptorStore::snapshot(ProvisioningSnapshot& out,ModuleSlot slot) {
    out={};std::unique_ptr<uint8_t[]> bytes(new(std::nothrow) uint8_t[EepromBytes]);
    if((slot!=ModuleSlot::A&&slot!=ModuleSlot::B)||!bytes||!capture(storage_,bytes.get(),out,slot==ModuleSlot::A?'A':'B'))return false;
    ReadOnlyImage image(bytes.get());HardwareDescriptorStore store(image);
    std::unique_ptr<uint8_t[]> payload(new(std::nothrow) uint8_t[MaximumPayloadSize]);
    std::unique_ptr<HardwareDescriptor> descriptor(new(std::nothrow) HardwareDescriptor);
    if(!payload||!descriptor){out={};return false;}
    const auto read=store.read(payload.get(),MaximumPayloadSize);out.status=ProvisioningSnapshotStatus::Corrupt;
    if(read.status==HardwareDescriptorStoreStatus::NotProvisioned){
        bool ff=true,zero=true;for(size_t i=0;i<EepromBytes;++i){ff&=bytes[i]==255;zero&=bytes[i]==0;}
        if(ff||zero)out.status=ProvisioningSnapshotStatus::Empty;
        return true;
    }
    if(read.status!=HardwareDescriptorStoreStatus::Valid||HardwareDescriptorCodec::decode(payload.get(),read.envelope.payloadLength,HardwareDescriptorObjectKind::Module,*descriptor)!=HardwareDescriptorDecodeStatus::Valid)return true;
    if(!copy(out.name,descriptor->name)||!copy(out.serial,descriptor->serialNumber)||!copy(out.batch,descriptor->productionBatch)||!copy(out.date,descriptor->productionDate))return true;
    out.revision={descriptor->hardwareRevision.major,descriptor->hardwareRevision.minor};out.status=ProvisioningSnapshotStatus::Valid;return true;
}
const char* provisioningSnapshotStatusName(ProvisioningSnapshotStatus s){
    switch(s){case ProvisioningSnapshotStatus::Valid:return "Valid";case ProvisioningSnapshotStatus::Empty:return "Empty";case ProvisioningSnapshotStatus::Corrupt:return "Corrupt";default:return "Unavailable";}
}
}
