// EnvNode: see repository LICENSE.
#include "SdFileTransfer.h"
#include <cstring>
namespace EnvNode {
bool SdDeleteTicket::issue(const char* path,const char* token,const SdFileStamp& stamp,uint32_t now) {
    valid_=false;
    if(!path || !token || strlen(path)==0 || strlen(path)>240 || strlen(token)!=32 || stamp.media==0) return false;
    strcpy(path_,path);strcpy(token_,token);stamp_=stamp;issued_=now;valid_=true;return true;
}
bool SdDeleteTicket::consume(const char* path,const char* token,uint32_t now,SdFileStamp& expected) {
    const bool valid=valid_;valid_=false;
    if(!valid || !path || !token || now-issued_>=120000UL || strcmp(path,path_) || strcmp(token,token_)) return false;
    expected=stamp_;return true;
}
bool captureSdFileStamp(const SdFileMetadata& descriptor,const SdFileMetadata& path,uint64_t media,SdFileStamp& output) {
    if(!descriptor.regular || !path.regular || descriptor.bytes!=path.bytes || media==0) return false;
    output=SdFileStamp(path.bytes,path.modified,media);
    return true;
}
bool sameSdFileStamp(const SdFileStamp& a, const SdFileStamp& b) {
    return a.media!=0 && a.media==b.media && a.bytes==b.bytes && a.modified==b.modified;
}
SdDownloadPump::SdDownloadPump(ISdStoredFile& source, ISdDownloadSink& sink, uint32_t now)
    : source_(source), sink_(sink), expected_(source.stamp().bytes), started_(now), progress_(now) {}
SdTransferState SdDownloadPump::step(uint32_t now) {
    if (!sink_.connected()) return SdTransferState::Failed;
    if (sent_==expected_) return SdTransferState::Complete;
    if (now-started_>=600000UL || now-progress_>=10000UL) return SdTransferState::TimedOut;
    if (offset_==count_) {
        const uint64_t remaining=expected_-sent_;
        const size_t requested=remaining<sizeof(buffer_) ? static_cast<size_t>(remaining) : sizeof(buffer_);
        const int received=source_.read(buffer_, requested);
        if (received<=0 || static_cast<size_t>(received)>requested) return SdTransferState::Failed;
        count_=received; offset_=0;
    }
    const int written=sink_.send(buffer_+offset_, count_-offset_);
    if (written<0 || static_cast<size_t>(written)>count_-offset_) return SdTransferState::Failed;
    if (written>0) { offset_+=written; sent_+=written; progress_=now; }
    return sent_==expected_ ? SdTransferState::Complete : SdTransferState::More;
}
}
