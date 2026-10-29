// EnvNode: see repository LICENSE.
#include <unity.h>
#include <Arduino.h>
#include <cstring>
#include "BoardPortMap.h"
#include "BoardIdentityProfiles.h"
#include "BoardProvisioningService.h"
#include "BoardIdentityResolver.h"
#include "BoardProfile.h"
#include "IdentityRecordCrc.h"
using namespace EnvNode;
HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
size_t HardwareSerial::write(const uint8_t*,size_t n){return n;}
void pinMode(unsigned char,int) {}
void digitalWrite(unsigned char,int) {}
class Memory : public IIdentityStorage {
public:
    uint8_t bytes[4096];int writes=0,failAfter=-1;bool unavailable=false;
    Memory(){memset(bytes,255,sizeof(bytes));}
    bool read(uint16_t address,uint8_t* out,size_t n) override {
        if(unavailable||address+n>sizeof(bytes))return false;memcpy(out,bytes+address,n);return true;
    }
    bool write(uint16_t address,const uint8_t* in,size_t n) override {
        if(unavailable||address+n>sizeof(bytes)||(failAfter>=0&&writes>=failAfter))return false;
        ++writes;memcpy(bytes+address,in,n);return true;
    }
};
BoardPortMap mini() {
    BoardPortMap p;const auto* profile=boardIdentityProfile(BoardProfileId::EnvNodeMini,{0,8});
    TEST_ASSERT_NOT_NULL(profile);
    TEST_ASSERT_TRUE(BoardPortMapCodec::decode(profile->portMap,profile->portMapSize,p));
    setBoardSerialNumber(p.serial,"MAP-TEST");return p;
}
BoardIdentity identity(const BoardPortMap& p){return {static_cast<BoardProfileId>(p.profileId),p.revision,p.serial};}
BoardIdentityResolution resolve(Memory& m) {
    BoardIdentityStore enid(m);BoardPortMapStore maps(m);
    BoardIdentityResolver resolver(enid,BoardProfileId::EnvNodeMainboard,&maps);
    return resolver.resolve();
}
void test_all_generated_maps_round_trip_with_original_pin_mappings() {
    for(size_t i=0;i<boardIdentityProfileCount();++i){
        const auto* record=boardIdentityProfileAt(i);BoardPortMap p;
        TEST_ASSERT_TRUE(BoardPortMapCodec::decode(record->portMap,record->portMapSize,p));
        const auto* original=boardProfile(record->id,record->revision);TEST_ASSERT_NOT_NULL(original);
        TEST_ASSERT_TRUE(boardPortMapRuntimeSupported(p,{0,8,2}));
        TEST_ASSERT_EQUAL(original->gpioCount,p.gpioCount);TEST_ASSERT_EQUAL(original->i2cBusCount,p.busCount);
        for(size_t g=0;g<p.gpioCount;++g){TEST_ASSERT_EQUAL(original->gpios[g].resource.number,p.gpios[g].pin);TEST_ASSERT_EQUAL(static_cast<uint8_t>(original->gpios[g].capabilities),p.gpios[g].capabilities);}
        uint8_t bytes[1024];size_t n=0;TEST_ASSERT_TRUE(BoardPortMapCodec::encode(p,bytes,sizeof(bytes),n));
        TEST_ASSERT_EQUAL(record->portMapSize,n);TEST_ASSERT_EQUAL_UINT8_ARRAY(record->portMap,bytes,n);
        for(size_t length=0;length<n;++length){BoardPortMap rejected;TEST_ASSERT_FALSE(BoardPortMapCodec::decode(bytes,length,rejected));}
    }
}
void test_map_store_preserves_identity_and_unrelated_regions() {
    Memory m;memset(m.bytes,0x5a,256);memset(m.bytes+3840,0xa5,256);
    uint8_t head[256],tail[256];memcpy(head,m.bytes,256);memcpy(tail,m.bytes+3840,256);
    BoardPortMapStore maps(m);auto p=mini();
    TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,maps.write(p).status);
    p.slots[0].aux1=17;TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,maps.write(p).status);
    BoardPortMap read;const auto result=maps.read(read);TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,result.status);
    TEST_ASSERT_EQUAL(2,result.generation);TEST_ASSERT_EQUAL(17,read.slots[0].aux1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(head,m.bytes,256);TEST_ASSERT_EQUAL_UINT8_ARRAY(tail,m.bytes+3840,256);
}
void test_each_interrupted_bank_write_keeps_previous_map() {
    for(int fail=0;fail<4;++fail){Memory m;BoardPortMapStore maps(m);auto old=mini();
        TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,maps.write(old).status);
        auto changed=old;changed.slots[0].aux1=17;m.failAfter=m.writes+fail;
        TEST_ASSERT_EQUAL(BoardPortMapStatus::WriteFailed,maps.write(changed).status);
        m.failAfter=-1;BoardPortMap read;TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,maps.read(read).status);
        TEST_ASSERT_EQUAL(old.slots[0].aux1,read.slots[0].aux1);
        TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,maps.write(changed).status);
        TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,maps.read(read).status);TEST_ASSERT_EQUAL(17,read.slots[0].aux1);
    }
}
void test_unknown_revision_and_family_resolve_without_builtin_tables() {
    Memory m;BoardIdentityStore enid(m);BoardPortMapStore maps(m);BoardProvisioningService writer(enid,&maps);
    auto p=mini();p.profileId=42;p.revision={0,77};p.slots[0].aux1=17;
    TEST_ASSERT_NULL(boardProfile(static_cast<BoardProfileId>(42),p.revision));
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::Success,writer.provision(identity(p),true,&p).status);
    const auto result=resolve(m);TEST_ASSERT_EQUAL(BoardIdentitySource::PortMap,result.source);TEST_ASSERT_TRUE(result.normalRuntimeAllowed);
    TEST_ASSERT_EQUAL(42,encodeBoardProfileId(result.identity.profileId));TEST_ASSERT_EQUAL(77,result.identity.revision.minor);
    // Older readers remain strict and cannot silently operate an unknown family.
    TEST_ASSERT_EQUAL(BoardIdentityStatus::UnknownBoardProfile,enid.read().status);
}
void test_legacy_identity_only_keeps_existing_boot_path() {
    Memory m;BoardIdentityStore enid(m);auto p=mini();TEST_ASSERT_EQUAL(BoardIdentityWriteStatus::Success,enid.write(identity(p)));
    const auto result=resolve(m);TEST_ASSERT_EQUAL(BoardIdentitySource::EEPROM,result.source);TEST_ASSERT_TRUE(result.normalRuntimeAllowed);
}
void test_corrupt_or_mismatched_map_never_falls_back_to_compiled_mapping() {
    Memory m;BoardIdentityStore enid(m);BoardPortMapStore maps(m);BoardProvisioningService writer(enid,&maps);auto p=mini();
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::Success,writer.provision(identity(p),true,&p).status);
    m.bytes[0x0800+25]^=1;
    auto result=resolve(m);TEST_ASSERT_EQUAL(BoardIdentitySource::InvalidPortMap,result.source);TEST_ASSERT_FALSE(result.normalRuntimeAllowed);
    TEST_ASSERT_EQUAL(BoardPortMapStatus::Corrupt,result.portMapStatus);
    TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,maps.write(p).status);
    auto stale=identity(p);setBoardSerialNumber(stale.serialNumber,"DIFFERENT");TEST_ASSERT_EQUAL(BoardIdentityWriteStatus::Success,enid.write(stale));
    result=resolve(m);TEST_ASSERT_EQUAL(BoardIdentitySource::InvalidPortMap,result.source);TEST_ASSERT_FALSE(result.normalRuntimeAllowed);
}
void test_provisioning_does_not_require_runtime_version_or_usable_pins() {
    Memory m;BoardIdentityStore enid(m);BoardPortMapStore maps(m);BoardProvisioningService writer(enid,&maps);auto p=mini();
    p.minimumFirmware={9,0,0};TEST_ASSERT_EQUAL(BoardProvisioningStatus::Success,writer.provision(identity(p),true,&p).status);
    TEST_ASSERT_FALSE(resolve(m).normalRuntimeAllowed);
    p.minimumFirmware={0,8,2};p.gpios[0].pin=6;
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::Success,writer.provision(identity(p),true,&p).status);
    TEST_ASSERT_FALSE(resolve(m).normalRuntimeAllowed);
    p.gpios[0].pin=48;
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::Success,writer.provision(identity(p),true,&p).status);
    TEST_ASSERT_FALSE(resolve(m).normalRuntimeAllowed);
    for(uint8_t reserved : {1,3}) {
        p.gpios[0].pin=reserved;
        TEST_ASSERT_EQUAL(BoardProvisioningStatus::Success,writer.provision(identity(p),true,&p).status);
        TEST_ASSERT_FALSE(resolve(m).normalRuntimeAllowed);
    }
    p=mini();p.gpios[0].capabilities|=64;
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::Success,writer.provision(identity(p),true,&p).status);
    TEST_ASSERT_FALSE(resolve(m).normalRuntimeAllowed);
}
void test_invalid_map_and_missing_confirmation_do_not_write() {
    Memory m;BoardIdentityStore enid(m);BoardPortMapStore maps(m);BoardProvisioningService writer(enid,&maps);auto p=mini();
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::ConfirmationRequired,writer.provision(identity(p),false,&p).status);
    p.gpios[1]=p.gpios[0];TEST_ASSERT_EQUAL(BoardProvisioningStatus::InvalidIdentity,writer.provision(identity(p),true,&p).status);
    TEST_ASSERT_EQUAL(0,m.writes);
}
void test_cross_record_interruption_fails_closed_and_retry_recovers() {
    Memory m;BoardIdentityStore enid(m);BoardPortMapStore maps(m);BoardProvisioningService writer(enid,&maps);auto p=mini();
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::Success,writer.provision(identity(p),true,&p).status);
    auto changed=p;changed.revision={0,99};m.failAfter=m.writes+4;
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::WriteFailed,writer.provision(identity(changed),true,&changed).status);
    TEST_ASSERT_FALSE(resolve(m).normalRuntimeAllowed);
    m.failAfter=-1;TEST_ASSERT_EQUAL(BoardProvisioningStatus::Success,writer.provision(identity(changed),true,&changed).status);
    TEST_ASSERT_TRUE(resolve(m).normalRuntimeAllowed);
}
void test_builtin_provisioning_migrates_enid_only_board_without_erasing_other_data() {
    Memory m;BoardIdentityStore enid(m);BoardPortMapStore maps(m);auto p=mini();
    TEST_ASSERT_EQUAL(BoardIdentityWriteStatus::Success,enid.write(identity(p)));
    memset(m.bytes+64,0x5a,192);
    BoardProvisioningService writer(enid,&maps);
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::Success,writer.provision(identity(p),true).status);
    BoardPortMap decoded;TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,maps.read(decoded).status);
    TEST_ASSERT_EQUAL_STRING(p.serial.value,decoded.serial.value);TEST_ASSERT_TRUE(resolve(m).normalRuntimeAllowed);
    for(size_t i=64;i<256;++i)TEST_ASSERT_EQUAL_HEX8(0x5a,m.bytes[i]);
}
void test_failed_map_readback_does_not_change_enid_or_report_success() {
    class CorruptingMemory : public Memory {
        bool write(uint16_t address,const uint8_t* data,size_t n) override {
            const bool ok=Memory::write(address,data,n);
            if(ok&&address==0x800&&n==4&&data[0]=='E')bytes[0x800+24]^=1;
            return ok;
        }
    } m;
    BoardIdentityStore enid(m);BoardPortMapStore maps(m);BoardProvisioningService writer(enid,&maps);auto p=mini();
    TEST_ASSERT_EQUAL(BoardProvisioningStatus::ReadbackFailed,writer.provision(identity(p),true).status);
    TEST_ASSERT_EQUAL(BoardIdentityStatus::NotProvisioned,enid.read().status);
}
void test_unavailable_map_storage_never_activates_a_compiled_mapping() {
    Memory m;m.unavailable=true;
    TEST_ASSERT_FALSE(resolve(m).normalRuntimeAllowed);
    TEST_ASSERT_EQUAL(BoardPortMapStatus::Unavailable,resolve(m).portMapStatus);
}
void test_unknown_map_format_is_not_replaced_by_an_older_bank() {
    Memory m;BoardPortMapStore maps(m);auto p=mini();
    TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,maps.write(p).status);
    TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,maps.write(p).status);
    m.bytes[0x100+4]=2;
    BoardPortMap decoded;TEST_ASSERT_EQUAL(BoardPortMapStatus::InvalidMap,maps.read(decoded).status);
    const int writes=m.writes;TEST_ASSERT_EQUAL(BoardPortMapStatus::InvalidMap,maps.write(p).status);
    TEST_ASSERT_EQUAL(writes,m.writes);
}
void test_generation_wraparound_selects_the_newest_committed_bank() {
    Memory m;BoardPortMapStore maps(m);auto p=mini();TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,maps.write(p).status);
    auto* header=m.bytes+0x800;memset(header+8,255,4);
    const uint32_t crc=calculateIdentityRecordCrc32(header,16);
    for(size_t i=0;i<4;++i)header[16+i]=crc>>(8*i);
    p.slots[0].aux1=17;const auto result=maps.write(p);TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,result.status);
    TEST_ASSERT_EQUAL_UINT32(0,result.generation);BoardPortMap decoded;
    TEST_ASSERT_EQUAL(BoardPortMapStatus::Valid,maps.read(decoded).status);TEST_ASSERT_EQUAL(17,decoded.slots[0].aux1);
}
void test_runtime_owns_loaded_data_after_decode_buffer_is_released() {
    auto p=mini();p.profileId=42;p.revision={0,77};p.slots[0].aux1=17;
    TEST_ASSERT_TRUE(selectCurrentBoardPortMap(p));
    strcpy(p.name,"Overwritten");p.slots[0].aux1=4;
    TEST_ASSERT_EQUAL_STRING("EnvNode Mini",currentBoardProfile().displayName);
    TEST_ASSERT_EQUAL(17,boardModuleSlot(currentBoardProfile(),ModuleSlot::A)->auxGpio1.number);
    TEST_ASSERT_EQUAL(42,encodeBoardProfileId(currentBoardProfile().id));
}
int main(int,char**) {
    UNITY_BEGIN();
    RUN_TEST(test_all_generated_maps_round_trip_with_original_pin_mappings);
    RUN_TEST(test_map_store_preserves_identity_and_unrelated_regions);
    RUN_TEST(test_each_interrupted_bank_write_keeps_previous_map);
    RUN_TEST(test_unknown_revision_and_family_resolve_without_builtin_tables);
    RUN_TEST(test_legacy_identity_only_keeps_existing_boot_path);
    RUN_TEST(test_corrupt_or_mismatched_map_never_falls_back_to_compiled_mapping);
    RUN_TEST(test_provisioning_does_not_require_runtime_version_or_usable_pins);
    RUN_TEST(test_invalid_map_and_missing_confirmation_do_not_write);
    RUN_TEST(test_cross_record_interruption_fails_closed_and_retry_recovers);
    RUN_TEST(test_builtin_provisioning_migrates_enid_only_board_without_erasing_other_data);
    RUN_TEST(test_failed_map_readback_does_not_change_enid_or_report_success);
    RUN_TEST(test_unavailable_map_storage_never_activates_a_compiled_mapping);
    RUN_TEST(test_unknown_map_format_is_not_replaced_by_an_older_bank);
    RUN_TEST(test_generation_wraparound_selects_the_newest_committed_bank);
    RUN_TEST(test_runtime_owns_loaded_data_after_decode_buffer_is_released);
    return UNITY_END();
}
