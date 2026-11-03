// EnvNode: see repository LICENSE.
#include "ProfileRepository.h"
#include <cstring>
#include <new>
namespace EnvNode { namespace Testing {
const RemoteProfileEntry* ProfileRepository::remoteEntry(const char* id) const {
    if (id) for(size_t i=0;i<catalog_.count;++i) if(!strcmp(id,catalog_.entries[i].id)) return &catalog_.entries[i];
    return nullptr;
}
void ProfileRepository::service(bool ready, bool running) {
    if (!pending_ || !ready || running) return;
    pending_=false;
    std::unique_ptr<uint8_t[]> bytes;size_t size=0;
    std::unique_ptr<RemoteCatalog> candidate(new(std::nothrow) RemoteCatalog());
    if (!candidate || !transport_.fetch("catalog.cbor",MaximumCatalogBytes,nullptr,bytes,size)
        || !decodeCatalog(bytes.get(),size,*candidate)) { status_="unavailable"; ++revision_; return; }
    for(size_t i=0;i<candidate->count;++i) {
        const auto* prior=remoteEntry(candidate->entries[i].id);
        const auto& entry=candidate->entries[i];
        for(const auto* held : {active_.get(),prepared_.get()}) {
            if(held && !strcmp(held->board.id,entry.id) && strcmp(held->contentSha256,entry.sha256)) {
                status_="revision-conflict";++revision_;return;
            }
        }
        if (prior && (strcmp(prior->sha256,entry.sha256) || strcmp(prior->name,entry.name)
            || strcmp(prior->identity,entry.identity) || strcmp(prior->pcb,entry.pcb)
            || strcmp(prior->filename,entry.filename) || prior->bytes!=entry.bytes)) {
            status_="revision-conflict"; ++revision_; return;
        }
    }
    catalog_=*candidate; status_="ready"; ++revision_;
}
size_t ProfileRepository::count() const {
    size_t n=catalog_.count;
    for(size_t i=0;i<boardCount();++i) if(!remoteEntry(boardAt(i)->id)) ++n;
    return n;
}
bool ProfileRepository::entryAt(size_t index, RemoteProfileEntry& entry, bool& remote) const {
    if(index<catalog_.count) { entry=catalog_.entries[index]; remote=true; return true; }
    index-=catalog_.count;
    for(size_t i=0;i<boardCount();++i) {
        const auto& b=*boardAt(i); if(remoteEntry(b.id)) continue;
        if(index--) continue;
        entry={};
        strncpy(entry.id,b.id,sizeof(entry.id)-1); strncpy(entry.name,b.name,sizeof(entry.name)-1);
        strncpy(entry.identity,b.identityRevision,sizeof(entry.identity)-1); strncpy(entry.pcb,b.pcbRevision,sizeof(entry.pcb)-1);
        remote=false; return true;
    }
    return false;
}
const OwnedProfile* ProfileRepository::active(const char* id) const {
    return id && active_ && !strcmp(active_->board.id,id) ? active_.get() : nullptr;
}
const OwnedProfile* ProfileRepository::prepare(const char* id, bool allowed) {
    if (const auto* p=active(id)) return p;
    if(prepared_ && id && !strcmp(prepared_->board.id,id)) return prepared_.get();
    const auto* entry=remoteEntry(id);
    if(!entry || !allowed) return nullptr;
    std::unique_ptr<uint8_t[]> bytes;size_t size=0;
    std::unique_ptr<OwnedProfile> candidate(new(std::nothrow) OwnedProfile());
    if(!candidate || !transport_.fetch(entry->filename,MaximumProfileBytes,entry->sha256,bytes,size)
        || !decodeProfile(bytes.get(),size,*entry,*candidate)) {
        status_="profile-fallback"; ++revision_; return nullptr;
    }
    prepared_=std::move(candidate); return prepared_.get();
}
bool ProfileRepository::load(const char* id, TestRunner& runner) {
    if(runner.phase()==Phase::Running) return false;
    if(const auto* profile=prepare(id,true)) {
        if(!runner.load(profile->board)) return false;
        if(profile!=active_.get()) active_=std::move(prepared_);
        return true;
    }
    const auto* builtin=findBoard(id);
    if(!builtin || !runner.load(*builtin)) return false;
    active_.reset();return true;
}
} }
