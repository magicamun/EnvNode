// EnvNode: see repository LICENSE.
#include "BoardPortMap.h"
#include "IdentityRecordCrc.h"
#include <cstring>
namespace EnvNode {
namespace {
constexpr uint16_t Addresses[]={0x0100,0x0800};
uint16_t u16(const uint8_t* p){return p[0]|(uint16_t(p[1])<<8);}
uint32_t u32(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
void put16(uint8_t* p,uint16_t n){p[0]=n;p[1]=n>>8;}
void put32(uint8_t* p,uint32_t n){for(size_t i=0;i<4;++i)p[i]=n>>(8*i);}
bool newer(uint32_t a,uint32_t b){return static_cast<int32_t>(a-b)>0;}
}
BoardPortMapStore::Bank BoardPortMapStore::inspect(uint16_t address) {
    Bank b;uint8_t header[24];
    if(!storage_.read(address,header,sizeof(header))){b.available=false;return b;}
    // An erased bank is absent; any other header is evidence of a failed/corrupt map.
    bool erased=true,zero=true;
    for(uint8_t c:header){erased=erased&&c==255;zero=zero&&c==0;}
    b.present=!erased&&!zero;
    b.unsupported=!memcmp(header,"ENPM",4)&&(header[4]!=1||header[5]!=24);
    if(memcmp(header,"ENPM",4)||header[4]!=1||header[5]!=24||u32(header+20)
        ||u32(header+16)!=calculateIdentityRecordCrc32(header,16))return b;
    b.length=u16(header+6);b.generation=u32(header+8);
    if(!b.length||b.length>BoardPortMapCodec::MaximumBytes)return b;
    IdentityRecordCrc32 crc;uint8_t chunk[32];
    for(size_t offset=0;offset<b.length;){size_t n=b.length-offset;if(n>sizeof(chunk))n=sizeof(chunk);
        if(!storage_.read(address+24+offset,chunk,n)){b.available=false;return b;}crc.update(chunk,n);offset+=n;}
    b.valid=crc.value()==u32(header+12);return b;
}
BoardPortMapResult BoardPortMapStore::read(BoardPortMap& map) {
    const Bank a=inspect(Addresses[0]),b=inspect(Addresses[1]);BoardPortMapResult result;
    if(!a.available||!b.available){result.status=BoardPortMapStatus::Unavailable;return result;}
    if(a.unsupported||b.unsupported){result.status=BoardPortMapStatus::InvalidMap;return result;}
    if(!a.valid&&!b.valid){result.status=(a.present||b.present)?BoardPortMapStatus::Corrupt:BoardPortMapStatus::NotProvisioned;return result;}
    const bool second=b.valid&&(!a.valid||newer(b.generation,a.generation));const Bank& selected=second?b:a;
    uint8_t bytes[BoardPortMapCodec::MaximumBytes];
    result.generation=selected.generation;
    if(!storage_.read(Addresses[second?1:0]+24,bytes,selected.length)){result.status=BoardPortMapStatus::Unavailable;return result;}
    result.status=BoardPortMapCodec::decode(bytes,selected.length,map)?BoardPortMapStatus::Valid:BoardPortMapStatus::InvalidMap;
    return result;
}
BoardPortMapResult BoardPortMapStore::write(const BoardPortMap& map) {
    BoardPortMapResult result;uint8_t bytes[BoardPortMapCodec::MaximumBytes];size_t size=0;
    if(!BoardPortMapCodec::encode(map,bytes,sizeof(bytes),size)){result.status=BoardPortMapStatus::InvalidMap;return result;}
    const Bank a=inspect(Addresses[0]),b=inspect(Addresses[1]);
    if(!a.available||!b.available){result.status=BoardPortMapStatus::Unavailable;return result;}
    if(a.unsupported||b.unsupported){result.status=BoardPortMapStatus::InvalidMap;return result;}
    const bool activeB=b.valid&&(!a.valid||newer(b.generation,a.generation));
    const size_t target=(a.valid||b.valid)?(activeB?0:1):1;
    result.generation=(a.valid||b.valid)?(activeB?b.generation:a.generation)+1U:1;
    uint8_t header[24]={};memcpy(header,"ENPM",4);header[4]=1;header[5]=24;
    put16(header+6,size);put32(header+8,result.generation);put32(header+12,calculateIdentityRecordCrc32(bytes,size));
    put32(header+16,calculateIdentityRecordCrc32(header,16));
    const uint16_t address=Addresses[target];const uint8_t invalid[4]={};
    if(!storage_.write(address,invalid,4)||!storage_.write(address+24,bytes,size)
        ||!storage_.write(address+4,header+4,20)||!storage_.write(address,header,4)) {
        result.status=BoardPortMapStatus::WriteFailed;return result;
    }
    const Bank verified=inspect(address);
    if(!verified.available||!verified.valid||verified.generation!=result.generation||verified.length!=size){result.status=BoardPortMapStatus::VerificationFailed;return result;}
    uint8_t chunk[32];
    for(size_t offset=0;offset<size;){size_t n=size-offset;if(n>sizeof(chunk))n=sizeof(chunk);
        if(!storage_.read(address+24+offset,chunk,n)||memcmp(chunk,bytes+offset,n)){result.status=BoardPortMapStatus::VerificationFailed;return result;}offset+=n;}
    result.status=BoardPortMapStatus::Valid;return result;
}
const char* boardPortMapStatusName(BoardPortMapStatus status) {
    switch(status){
    case BoardPortMapStatus::Valid:return "Valid";
    case BoardPortMapStatus::NotProvisioned:return "NotProvisioned";
    case BoardPortMapStatus::Unavailable:return "Unavailable";
    case BoardPortMapStatus::Corrupt:return "Corrupt";
    case BoardPortMapStatus::InvalidMap:return "InvalidMap";
    case BoardPortMapStatus::WriteFailed:return "WriteFailed";
    case BoardPortMapStatus::VerificationFailed:return "VerificationFailed";
    }return "Unknown";
}
}
