// EnvNode: see repository LICENSE.
#pragma once
#include <cstdint>
namespace EnvNode {
struct SdFatGeometry {
    uint64_t dataBytes=0;
    uint32_t clusters=0;
    uint32_t clusterBytes=0;
    uint32_t infoSector=UINT32_MAX;
};
bool parseSdFatBoot(const uint8_t* sector,uint32_t start,uint32_t cardSectors,SdFatGeometry& geometry);
bool parseSdFatFreeHint(const uint8_t* sector,const SdFatGeometry& geometry,uint64_t& usedBytes);
}
