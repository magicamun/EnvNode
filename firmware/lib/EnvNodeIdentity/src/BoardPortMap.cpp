// EnvNode: see repository LICENSE.
#include "BoardPortMap.h"
#include "CompactCborCodec.h"
#include "IdentityRecordCrc.h"
#include <cstring>
namespace EnvNode {
namespace {
bool array(CompactCborReader& r,size_t n) {size_t count=0;return r.enterArray(count)&&count==n;}
bool number(CompactCborReader& r,uint8_t& out) {uint64_t n=0;if(!r.readUnsigned(n)||n>255)return false;out=n;return true;}
bool text(CompactCborReader& r,char* out,size_t capacity) {
    const char* p=nullptr;size_t n=0;
    if(!r.readText(p,n)||n>=capacity)return false;
    for(size_t i=0;i<n;++i)if(static_cast<uint8_t>(p[i])<32||p[i]==127)return false;
    memcpy(out,p,n);out[n]=0;return true;
}
bool valid(const BoardPortMap& p) {
    if(!memchr(p.name,0,sizeof(p.name))||!memchr(p.serial.value,0,sizeof(p.serial.value)))return false;
    if(p.profileId==255||!p.name[0]||!p.gpioCount||p.gpioCount>32||!p.busCount||p.busCount>2||p.slotCount>2
        ||(p.revision.major==255&&p.revision.minor==255)||!isValidBoardSerialNumber(p.serial.value))return false;
    uint8_t buses=0,slots=0;
    for(size_t i=0;i<p.gpioCount;++i) {
        const auto& g=p.gpios[i];if(g.pin==255||!g.capabilities)return false;
        for(size_t j=0;j<i;++j)if(p.gpios[j].pin==g.pin)return false;
    }
    for(size_t i=0;i<p.busCount;++i) {
        const auto& b=p.buses[i];if(b.bus>1||b.sda==255||b.scl==255||b.sda==b.scl||(buses&(1<<b.bus)))return false;
        buses|=1<<b.bus;
    }
    if(!(buses&1))return false;
    for(size_t i=0;i<p.slotCount;++i) {
        const auto& s=p.slots[i];if(s.slot>1||s.address!=(s.slot?0x53:0x52)||s.aux1==255||s.aux2==255||s.cs==255
            ||s.aux1==s.aux2||s.aux1==s.cs||s.aux2==s.cs||(slots&(1<<s.slot)))return false;
        slots|=1<<s.slot;
    }
    return p.spiClock!=255&&p.spiMiso!=255&&p.spiMosi!=255
        &&p.spiClock!=p.spiMiso&&p.spiClock!=p.spiMosi&&p.spiMiso!=p.spiMosi;
}
}
bool BoardPortMapCodec::encode(const BoardPortMap& p,uint8_t* bytes,size_t capacity,size_t& size) {
    size=0;if(!valid(p))return false;
    CompactCborWriter w(bytes,capacity);w.beginArray(10);w.writeUnsigned(1);w.writeUnsigned(p.profileId);
    w.beginArray(2);w.writeUnsigned(p.revision.major);w.writeUnsigned(p.revision.minor);
    w.writeText(p.name);w.beginArray(3);w.writeUnsigned(p.minimumFirmware.major);w.writeUnsigned(p.minimumFirmware.minor);w.writeUnsigned(p.minimumFirmware.patch);
    w.writeText(p.serial.value);w.beginArray(p.gpioCount);
    for(size_t i=0;i<p.gpioCount;++i){w.beginArray(2);w.writeUnsigned(p.gpios[i].pin);w.writeUnsigned(p.gpios[i].capabilities);}
    w.beginArray(p.busCount);
    for(size_t i=0;i<p.busCount;++i){w.beginArray(3);w.writeUnsigned(p.buses[i].bus);w.writeUnsigned(p.buses[i].sda);w.writeUnsigned(p.buses[i].scl);}
    w.beginArray(p.slotCount);
    for(size_t i=0;i<p.slotCount;++i){const auto& s=p.slots[i];w.beginArray(6);w.writeUnsigned(s.slot);w.writeUnsigned(s.address);w.writeUnsigned(s.aux1);w.writeUnsigned(s.aux2);w.writeUnsigned(s.cs);w.writeBoolean(s.fiveVolt);}
    w.beginArray(3);w.writeUnsigned(p.spiClock);w.writeUnsigned(p.spiMiso);w.writeUnsigned(p.spiMosi);
    size=w.size();return w.status()==CompactCborStatus::Success&&size<=MaximumBytes;
}
bool BoardPortMapCodec::decode(const uint8_t* bytes,size_t size,BoardPortMap& p) {
    if(!bytes||!size||size>MaximumBytes)return false;
    p=BoardPortMap();CompactCborReader r(bytes,size);uint64_t version=0;
    if(!array(r,10)||!r.readUnsigned(version)||version!=1||!number(r,p.profileId)||!array(r,2)
        ||!number(r,p.revision.major)||!number(r,p.revision.minor)||!text(r,p.name,sizeof(p.name))||!array(r,3))return false;
    uint16_t* versions[]={&p.minimumFirmware.major,&p.minimumFirmware.minor,&p.minimumFirmware.patch};
    for(auto* v:versions){uint64_t n=0;if(!r.readUnsigned(n)||n>65535)return false;*v=n;}
    if(!text(r,p.serial.value,sizeof(p.serial.value))||!r.enterArray(p.gpioCount)||p.gpioCount>32)return false;
    for(size_t i=0;i<p.gpioCount;++i)if(!array(r,2)||!number(r,p.gpios[i].pin)||!number(r,p.gpios[i].capabilities))return false;
    if(!r.enterArray(p.busCount)||p.busCount>2)return false;
    for(size_t i=0;i<p.busCount;++i)if(!array(r,3)||!number(r,p.buses[i].bus)||!number(r,p.buses[i].sda)||!number(r,p.buses[i].scl))return false;
    if(!r.enterArray(p.slotCount)||p.slotCount>2)return false;
    for(size_t i=0;i<p.slotCount;++i){auto& s=p.slots[i];if(!array(r,6)||!number(r,s.slot)||!number(r,s.address)||!number(r,s.aux1)||!number(r,s.aux2)||!number(r,s.cs)||!r.readBoolean(s.fiveVolt))return false;}
    return array(r,3)&&number(r,p.spiClock)&&number(r,p.spiMiso)&&number(r,p.spiMosi)&&!r.remaining()&&valid(p);
}
}
