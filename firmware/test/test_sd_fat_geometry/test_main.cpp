// EnvNode: see repository LICENSE.
#include <unity.h>
#include "SdFatGeometry.h"
#include "SdCardWebView.h"
#include <cstring>
using namespace EnvNode;
void pinMode(unsigned char,int) {} void digitalWrite(unsigned char,int) {}
void setUp(){} void tearDown(){}
namespace {
void put16(uint8_t* p,uint16_t v){p[0]=v;p[1]=v>>8;}
void put32(uint8_t* p,uint32_t v){put16(p,v);put16(p+2,v>>16);}
void fat32(uint8_t* s) {
    memset(s,0,512);s[0]=0xEB;s[510]=0x55;s[511]=0xAA;
    put16(s+11,512);s[13]=8;put16(s+14,32);s[16]=2;
    put32(s+32,15500000);put32(s+36,16000);put16(s+48,1);
}
void test_geometry_without_fat_scan() {
    uint8_t sector[512];fat32(sector);SdFatGeometry geometry;
    TEST_ASSERT_TRUE(parseSdFatBoot(sector,2048,15502048,geometry));
    TEST_ASSERT_EQUAL_UINT(2049,geometry.infoSector);
    TEST_ASSERT_EQUAL_UINT(1933496,geometry.clusters);
    TEST_ASSERT_EQUAL_UINT64(7919599616ULL,geometry.dataBytes);
    TEST_ASSERT_EQUAL_UINT(4096,geometry.clusterBytes);
}
void test_fat16_has_capacity_but_no_free_hint() {
    uint8_t sector[512]={};sector[0]=0xEB;sector[510]=0x55;sector[511]=0xAA;
    put16(sector+11,512);sector[13]=8;put16(sector+14,1);sector[16]=2;
    put16(sector+17,512);put32(sector+32,500000);put16(sector+22,250);
    SdFatGeometry geometry;TEST_ASSERT_TRUE(parseSdFatBoot(sector,0,500000,geometry));
    TEST_ASSERT_EQUAL_UINT((500000-1-500-32)/8,geometry.clusters);
    TEST_ASSERT_EQUAL_UINT(UINT32_MAX,geometry.infoSector);
}
void test_bad_or_out_of_card_geometry_is_rejected() {
    uint8_t sector[512];fat32(sector);SdFatGeometry geometry;
    TEST_ASSERT_FALSE(parseSdFatBoot(sector,2048,15502047,geometry));
    sector[13]=3;TEST_ASSERT_FALSE(parseSdFatBoot(sector,0,16000000,geometry));
    fat32(sector);put32(sector+36,1);TEST_ASSERT_FALSE(parseSdFatBoot(sector,0,16000000,geometry));
    fat32(sector);sector[510]=0;TEST_ASSERT_FALSE(parseSdFatBoot(sector,0,16000000,geometry));
}
void test_free_space_is_a_validated_optional_hint() {
    uint8_t sector[512];fat32(sector);SdFatGeometry geometry;
    TEST_ASSERT_TRUE(parseSdFatBoot(sector,2048,16000000,geometry));
    memset(sector,0,512);uint64_t used=123;
    TEST_ASSERT_FALSE(parseSdFatFreeHint(sector,geometry,used));
    put32(sector,0x41615252);put32(sector+484,0x61417272);put32(sector+508,0xAA550000);
    put32(sector+488,geometry.clusters-7);
    TEST_ASSERT_TRUE(parseSdFatFreeHint(sector,geometry,used));TEST_ASSERT_EQUAL_UINT64(7*4096,used);
    put32(sector+488,UINT32_MAX);TEST_ASSERT_FALSE(parseSdFatFreeHint(sector,geometry,used));
    put32(sector+488,geometry.clusters+1);TEST_ASSERT_FALSE(parseSdFatFreeHint(sector,geometry,used));
}
void test_ui_does_not_claim_unknown_free_space_is_zero() {
    SdDirectorySnapshot snapshot;snapshot.status=SdDirectoryStatus::Ready;
    snapshot.info.cardBytes=8000000000ULL;snapshot.info.filesystemBytes=7900000000ULL;
    snapshot.info.spaceUsageKnown=false;
    const String unknown=renderSdDirectory(snapshot,Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(unknown.c_str(),"Nicht verfügbar"));
    snapshot.info.spaceUsageKnown=true;snapshot.info.spaceUsageEstimated=true;
    const String approximate=renderSdDirectory(snapshot,Locale::GermanGermany);
    TEST_ASSERT_NOT_NULL(strstr(approximate.c_str(),"≈ "));
}
}
int main(){UNITY_BEGIN();
RUN_TEST(test_geometry_without_fat_scan);
RUN_TEST(test_bad_or_out_of_card_geometry_is_rejected);
RUN_TEST(test_fat16_has_capacity_but_no_free_hint);
RUN_TEST(test_free_space_is_a_validated_optional_hint);
RUN_TEST(test_ui_does_not_claim_unknown_free_space_is_zero);
return UNITY_END();}
