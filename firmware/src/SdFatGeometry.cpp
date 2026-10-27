// EnvNode: see repository LICENSE.
#include "SdFatGeometry.h"
namespace EnvNode {
namespace {
uint16_t le16(const uint8_t* p) {return p[0] | static_cast<uint16_t>(p[1])<<8;}
uint32_t le32(const uint8_t* p) {return le16(p) | static_cast<uint32_t>(le16(p+2))<<16;}
}
bool parseSdFatBoot(const uint8_t* s,uint32_t start,uint32_t cardSectors,SdFatGeometry& result) {
    if(!s || s[510]!=0x55 || s[511]!=0xAA || (s[0]!=0xEB && s[0]!=0xE9) || le16(s+11)!=512) return false;
    const uint32_t cluster=s[13],reserved=le16(s+14),fats=s[16],root=le16(s+17);
    const uint32_t total=le16(s+19) ? le16(s+19) : le32(s+32);
    const uint32_t fat16=le16(s+22),fat=fat16 ? fat16 : le32(s+36);
    if(!cluster || cluster>128 || (cluster&(cluster-1)) || !reserved || !fats || fats>2 || !fat
        || start>=cardSectors || total>cardSectors-start) return false;
    const uint64_t overhead=reserved+static_cast<uint64_t>(fats)*fat+(root*32+511)/512;
    if(total<=overhead) return false;
    const uint32_t clusters=(total-overhead)/cluster;
    if(!clusters || clusters>=0x0FFFFFF5U || (fat16==0 && (root!=0 || clusters<65525))) return false;
    const uint64_t fatCapacity=static_cast<uint64_t>(fat)*512*8;
    const unsigned bits=clusters<4085 ? 12 : clusters<65525 ? 16 : 32;
    if(static_cast<uint64_t>(clusters+2)*bits>fatCapacity) return false;
    SdFatGeometry parsed;parsed.clusters=clusters;parsed.clusterBytes=cluster*512;
    parsed.dataBytes=static_cast<uint64_t>(clusters)*parsed.clusterBytes;
    if(!fat16 && le16(s+48)>0 && le16(s+48)<reserved) parsed.infoSector=start+le16(s+48);
    result=parsed;return true;
}
bool parseSdFatFreeHint(const uint8_t* s,const SdFatGeometry& geometry,uint64_t& usedBytes) {
    if(!s || !geometry.clusters || le32(s)!=0x41615252U || le32(s+484)!=0x61417272U || le32(s+508)!=0xAA550000U) return false;
    const uint32_t free=le32(s+488);
    if(free==UINT32_MAX || free>geometry.clusters) return false;
    usedBytes=static_cast<uint64_t>(geometry.clusters-free)*geometry.clusterBytes;return true;
}
}
