#include "BoardProfile.h"
#include <initializer_list>
#include <new>
#include <cstring>
#include <cstdio>
#include "HardwareDescriptorCompatibility.h"

namespace EnvNode {
namespace {

#if defined(ENVNODE_BOARD_MAINBOARD)

#include "HardwareProfiles.generated.inc"

const BoardProfile* selectedProfile = nullptr;
bool selectionFrozen = false;

#else
#error "No EnvNode Board Profile selected. Define exactly one ENVNODE_BOARD_* build flag."
#endif

} // namespace

namespace {
bool usablePin(uint8_t pin,bool output) {
    const bool gpio=(pin<=5&&pin!=1&&pin!=3)||(pin>=12&&pin<=19)||(pin>=21&&pin<=23)||(pin>=25&&pin<=27)||(pin>=32&&pin<=36)||pin==39;
    return gpio&&(!output||pin<34);
}
struct OwnedBoardProfile {
    char name[96]; char labels[32][8];
    BoardGpioCapability gpios[32]; BoardI2CBusCapability buses[2]; BoardModuleSlotCapability slots[2];
    BoardProfile profile;
};
}
bool boardPortMapRuntimeSupported(const BoardPortMap& p,DescriptorSemanticVersion fw) {
    const auto& min=p.minimumFirmware;
    if(fw.major<min.major||(fw.major==min.major&&(fw.minor<min.minor||(fw.minor==min.minor&&fw.patch<min.patch))))return false;
    if(!p.gpioCount||p.gpioCount>32||!p.busCount||p.busCount>2||p.slotCount>2)return false;
    uint64_t gpioPins=0,busPins=0,slotPins=0;
    bool identityBus=false;
    for(size_t i=0;i<p.gpioCount;++i){const auto& g=p.gpios[i];
        if(!usablePin(g.pin,(g.capabilities&10)!=0)||!g.capabilities||(g.capabilities&~31)
            ||((g.capabilities&16)&&g.pin<32)||((gpioPins>>g.pin)&1))return false;
        gpioPins|=UINT64_C(1)<<g.pin;
    }
    uint8_t buses=0,slots=0;
    for(size_t i=0;i<p.busCount;++i){const auto& b=p.buses[i];
        if(b.bus>1||!usablePin(b.sda,true)||!usablePin(b.scl,true)||b.sda==b.scl||(buses&(1<<b.bus)))return false;
        const uint64_t pins=(UINT64_C(1)<<b.sda)|(UINT64_C(1)<<b.scl);
        if((pins&busPins)||(pins&gpioPins))return false;
        busPins|=pins;buses|=1<<b.bus;
        if(b.bus==0)identityBus=b.sda==21&&b.scl==22;
    }
    for(size_t i=0;i<p.slotCount;++i){const auto& s=p.slots[i];
        if(s.slot>1||s.address!=(s.slot?0x53:0x52)||(slots&(1<<s.slot))
            ||!usablePin(s.aux1,false)||!usablePin(s.aux2,false)||!usablePin(s.cs,true)
            ||s.aux1==s.aux2||s.aux1==s.cs||s.aux2==s.cs)return false;
        const uint64_t pins=(UINT64_C(1)<<s.aux1)|(UINT64_C(1)<<s.aux2)|(UINT64_C(1)<<s.cs);
        if(pins&(busPins|slotPins))return false;slotPins|=pins;slots|=1<<s.slot;
    }
    return identityBus&&usablePin(p.spiClock,true)&&usablePin(p.spiMiso,false)&&usablePin(p.spiMosi,true)
        &&p.spiClock!=p.spiMiso&&p.spiClock!=p.spiMosi&&p.spiMiso!=p.spiMosi
        &&!(((UINT64_C(1)<<p.spiClock)|(UINT64_C(1)<<p.spiMiso)|(UINT64_C(1)<<p.spiMosi))&(busPins|slotPins));
}
bool selectCurrentBoardPortMap(const BoardPortMap& p) {
    if(selectionFrozen||!boardPortMapRuntimeSupported(p,currentFirmwareDescriptorVersion()))return false;
    auto* owned=new(std::nothrow) OwnedBoardProfile;
    if(!owned)return false;
    memcpy(owned->name,p.name,sizeof(owned->name));
    for(size_t i=0;i<p.gpioCount;++i){snprintf(owned->labels[i],sizeof(owned->labels[i]),"GPIO%u",p.gpios[i].pin);
        owned->gpios[i]={GpioResource(p.gpios[i].pin),owned->labels[i],static_cast<GpioCapability>(p.gpios[i].capabilities)};}
    for(size_t i=0;i<p.busCount;++i){const auto& b=p.buses[i];owned->buses[i]={static_cast<I2CBus>(b.bus),GpioResource(b.sda),GpioResource(b.scl)};}
    for(size_t i=0;i<p.slotCount;++i){const auto& s=p.slots[i];owned->slots[i]={static_cast<ModuleSlot>(s.slot),s.address,GpioResource(s.aux1),GpioResource(s.aux2),GpioResource(s.cs),s.fiveVolt};}
    owned->profile={static_cast<BoardProfileId>(p.profileId),owned->name,p.revision,owned->gpios,p.gpioCount,owned->buses,p.busCount,
        owned->slots,p.slotCount,GpioResource(p.spiClock),GpioResource(p.spiMiso),GpioResource(p.spiMosi)};
    // One immutable boot-time selection owns the backing memory for the device lifetime.
    selectedProfile=&owned->profile;selectionFrozen=true;return true;
}

const BoardProfile& currentBoardProfile() {
    if (selectedProfile == nullptr) {
        selectedProfile = boardProfile(buildFallbackBoardProfileId());
    }
    selectionFrozen = true;
    return *selectedProfile;
}

const BoardProfile* boardProfile(BoardProfileId id) {
    for (size_t index = 0; index < boardProfileCount(); ++index) {
        if (BoardProfiles[index]->id == id) {
            return BoardProfiles[index];
        }
    }
    return nullptr;
}

const BoardProfile* boardProfile(BoardProfileId id, BoardRevision revision) {
    const auto* legacy=boardProfile(id);
    if(legacy && legacy->revision.major==revision.major && legacy->revision.minor==revision.minor)return legacy;
    for(const auto* p:RevisionProfiles)
        if(p->id==id && p->revision.major==revision.major && p->revision.minor==revision.minor)return p;
    return nullptr;
}

size_t boardProfileCount() {
    return sizeof(BoardProfiles) / sizeof(BoardProfiles[0]);
}

const BoardProfile* boardProfileAt(size_t index) {
    return index < boardProfileCount() ? BoardProfiles[index] : nullptr;
}

BoardProfileId buildFallbackBoardProfileId() {
#if defined(ENVNODE_BOARD_MAINBOARD)
    return BoardProfileId::EnvNodeMainboard;
#endif
}

bool selectCurrentBoardProfile(BoardProfileId id) {
    const BoardProfile* profile = boardProfile(id);
    if (profile == nullptr || selectionFrozen) {
        return false;
    }
    selectedProfile = profile;
    selectionFrozen = true;
    return true;
}

bool selectCurrentBoardProfile(BoardProfileId id, BoardRevision revision) {
    const BoardProfile* profile=boardProfile(id,revision);
    if(!profile || selectionFrozen)return false;
    selectedProfile=profile;selectionFrozen=true;return true;
}

const BoardModuleSlotCapability* boardModuleSlot(
    const BoardProfile& board,
    ModuleSlot slot) {
    for (size_t index = 0; index < board.moduleSlotCount; ++index) {
        if (board.moduleSlots[index].slot == slot) return &board.moduleSlots[index];
    }
    return nullptr;
}



} // namespace EnvNode
