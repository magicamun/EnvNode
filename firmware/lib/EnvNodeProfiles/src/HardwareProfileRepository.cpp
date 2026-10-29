// EnvNode: see repository LICENSE.
#include "HardwareProfileRepository.h"
#include "CompactCborCodec.h"
#include "HardwareDescriptorCodec.h"
#include <cstring>
#include <new>
namespace EnvNode {
namespace {
bool array(CompactCborReader& r,size_t n){size_t actual=0;return r.enterArray(actual)&&actual==n;}
bool text(CompactCborReader& r,char* out,size_t capacity,bool empty=false){
    const char* p=nullptr;size_t n=0;
    if(!r.readText(p,n)||n>=capacity||(!empty&&!n))return false;
    for(size_t i=0;i<n;++i)if(static_cast<uint8_t>(p[i])<32||p[i]==127)return false;
    memcpy(out,p,n);out[n]=0;return true;
}
bool number(CompactCborReader& r,uint64_t max,uint64_t& n){return r.readUnsigned(n)&&n<=max;}
bool digest(const char* p){if(strlen(p)!=64)return false;for(;*p;++p)if(!(*p>='0'&&*p<='9')&&!(*p>='a'&&*p<='f'))return false;return true;}
bool imagePath(const char* p){
    if(!*p)return true;
    if(strncmp(p,"assets/",7)||strstr(p,"..")||p[strlen(p)-1]=='/')return false;
    for(;*p;++p)if(!(*p>='a'&&*p<='z')&&!(*p>='A'&&*p<='Z')&&!(*p>='0'&&*p<='9')&&*p!='/'&&*p!='-'&&*p!='_'&&*p!='.')return false;
    return true;
}
bool same(const HardwareProfileEntry& a,const HardwareProfileEntry& b){
    return !strcmp(a.id,b.id)&&!strcmp(a.name,b.name)&&!strcmp(a.image,b.image)&&!strcmp(a.filename,b.filename)
        &&!strcmp(a.sha256,b.sha256)&&a.kind==b.kind&&a.bytes==b.bytes
        &&a.revision.major==b.revision.major&&a.revision.minor==b.revision.minor
        &&a.minimumFirmware.major==b.minimumFirmware.major&&a.minimumFirmware.minor==b.minimumFirmware.minor&&a.minimumFirmware.patch==b.minimumFirmware.patch;
}
bool versionMatches(const DescriptorSemanticVersion& a,const DescriptorSemanticVersion& b){return a.major==b.major&&a.minor==b.minor&&a.patch==b.patch;}
bool metadataMatches(const LoadedHardwareProfile& p){
    const auto& e=p.entry;
    if(e.kind==0){
        BoardPortMap map;
        return BoardPortMapCodec::decode(p.bytes.get(),p.size,map)&&!map.serial.value[0]
            &&!strcmp(map.name,e.name)&&map.revision.major==e.revision.major&&map.revision.minor==e.revision.minor&&versionMatches(map.minimumFirmware,e.minimumFirmware);
    }
    std::unique_ptr<HardwareDescriptor> d(new(std::nothrow) HardwareDescriptor);
    if(!d||HardwareDescriptorCodec::decode(p.bytes.get(),p.size,HardwareDescriptorObjectKind::Module,*d)!=HardwareDescriptorDecodeStatus::Valid)return false;
    for(uint8_t byte:d->instanceId)if(byte)return false;
    return d->serialNumber.empty()&&d->productionBatch.empty()&&d->productionDate.empty()
        &&d->name.equals(e.name)&&d->hardwareRevision.major==e.revision.major&&d->hardwareRevision.minor==e.revision.minor&&versionMatches(d->compatibility.minimumFirmwareVersion,e.minimumFirmware);
}
}
bool safeHardwareProfileFilename(const char* p){
    if(!p||!*p||strlen(p)>175||*p=='.'||strstr(p,".."))return false;
    for(;*p;++p)if(!(*p>='a'&&*p<='z')&&!(*p>='0'&&*p<='9')&&*p!='.'&&*p!='-')return false;
    return true;
}
bool decodeHardwareProfileCatalog(const uint8_t* bytes,size_t size,HardwareProfileCatalog& catalog){
    if(!bytes||!size||size>MaximumHardwareCatalogBytes)return false;
    CompactCborReader r(bytes,size);uint64_t version=0;size_t count=0;
    if(!array(r,2)||!number(r,1,version)||version!=1||!r.enterArray(count)||count>MaximumHardwareProfiles)return false;
    catalog.count=0;
    for(size_t i=0;i<count;++i){auto& e=catalog.entries[i];uint64_t n=0;
        if(!array(r,9)||!text(r,e.id,sizeof(e.id))||!safeHardwareProfileFilename(e.id)||!text(r,e.name,sizeof(e.name))
            ||!number(r,1,n))return false;
        e.kind=n;
        if(!array(r,2)||!number(r,255,n))return false;e.revision.major=n;
        if(!number(r,255,n))return false;e.revision.minor=n;
        if(!array(r,3)||!number(r,65535,n))return false;e.minimumFirmware.major=n;
        if(!number(r,65535,n))return false;e.minimumFirmware.minor=n;
        if(!number(r,65535,n))return false;e.minimumFirmware.patch=n;
        if(!text(r,e.image,sizeof(e.image),true)||!imagePath(e.image)||!text(r,e.filename,sizeof(e.filename))
            ||!safeHardwareProfileFilename(e.filename)||!number(r,MaximumHardwareProfileBytes,n)||!n)return false;
        e.bytes=n;
        if(!text(r,e.sha256,sizeof(e.sha256))||!digest(e.sha256))return false;
        char expected[176];const char* suffix=e.kind?".module.cbor":".portmap.cbor";
        const size_t needed=strlen(e.id)+1+64+strlen(suffix)+1;
        if(needed>sizeof(expected))return false;
        strcpy(expected,e.id);strcat(expected,".");strcat(expected,e.sha256);strcat(expected,suffix);
        if(strcmp(expected,e.filename))return false;
        for(size_t j=0;j<i;++j)if(!strcmp(e.id,catalog.entries[j].id))return false;
        ++catalog.count;
    }
    return !r.remaining();
}
bool HardwareProfileRepository::refresh(){
    attempted_=true;
    std::unique_ptr<uint8_t[]> bytes;size_t size=0;
    std::unique_ptr<HardwareProfileCatalog> next(new(std::nothrow) HardwareProfileCatalog);
    if(!next||!transport_.fetch("provisioning-catalog.cbor",MaximumHardwareCatalogBytes,nullptr,bytes,size)
        ||!decodeHardwareProfileCatalog(bytes.get(),size,*next)){status_="refresh-failed";return false;}
    for(size_t i=0;i<next->count;++i){const auto& candidate=next->entries[i];
        for(size_t j=0;j<count();++j)if(!strcmp(candidate.id,at(j)->id)&&!same(candidate,*at(j))){status_="identity-conflict";return false;}
        if(loaded_&&!strcmp(candidate.id,loaded_->entry.id)&&!same(candidate,loaded_->entry)){status_="identity-conflict";return false;}
    }
    catalog_=std::move(next);status_="ready";return true;
}
const LoadedHardwareProfile* HardwareProfileRepository::load(const char* id,const char* expectedDigest){
    if(!id||!expectedDigest)return nullptr;
    const HardwareProfileEntry* entry=nullptr;
    for(size_t i=0;i<count();++i)if(!strcmp(id,at(i)->id))entry=at(i);
    if(!entry||strcmp(entry->sha256,expectedDigest)){status_="selection-stale";return nullptr;}
    if(loaded_&&same(loaded_->entry,*entry)){status_="ready";return loaded_.get();}
    std::unique_ptr<LoadedHardwareProfile> next(new(std::nothrow) LoadedHardwareProfile);
    if(!next){status_="out-of-memory";return nullptr;}
    next->entry=*entry;
    if(!transport_.fetch(entry->filename,entry->bytes,entry->sha256,next->bytes,next->size)
        ||next->size!=entry->bytes||!metadataMatches(*next)){status_="profile-invalid";return nullptr;}
    loaded_=std::move(next);status_="ready";return loaded_.get();
}
}
