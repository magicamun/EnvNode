// EnvNode: see repository LICENSE.
#pragma once
#include <cstdint>
#include <cstddef>
namespace EnvNode {
struct SdFileStamp {
    SdFileStamp(uint64_t size=0,int64_t time=0,uint64_t card=0) : bytes(size),modified(time),media(card) {}
    uint64_t bytes; int64_t modified; uint64_t media;
};
struct SdFileMetadata {
    SdFileMetadata(uint64_t size=0,int64_t time=0,bool file=false) : bytes(size),modified(time),regular(file) {}
    uint64_t bytes; int64_t modified; bool regular;
};
// FAT fstat() omits timestamps; validate the descriptor but use path stat() time.
bool captureSdFileStamp(const SdFileMetadata& descriptor,const SdFileMetadata& path,uint64_t media,SdFileStamp& output);
bool sameSdFileStamp(const SdFileStamp& a, const SdFileStamp& b);
class SdDeleteTicket {
public:
    bool issue(const char* path,const char* token,const SdFileStamp& stamp,uint32_t now);
    bool consume(const char* path,const char* token,uint32_t now,SdFileStamp& expected);
    void clear() {valid_=false;}
private:
    char path_[241]={},token_[33]={}; SdFileStamp stamp_; uint32_t issued_=0; bool valid_=false;
};
class ISdStoredFile {
public:
    virtual ~ISdStoredFile() = default;
    virtual SdFileStamp stamp() const = 0;
    virtual int read(uint8_t* data, size_t size) = 0;
    virtual bool remove() = 0;
    virtual const char* lastError() const {return "unspecified";}
    virtual int errorCode() const {return 0;}
};
class ISdDownloadSink {
public:
    virtual ~ISdDownloadSink() = default;
    virtual bool connected() = 0;
    // Zero means temporarily busy, negative means permanent failure.
    virtual int send(const uint8_t* data, size_t size) = 0;
};
enum class SdTransferState { More, Complete, Failed, TimedOut };
class SdDownloadPump {
public:
    SdDownloadPump(ISdStoredFile& source, ISdDownloadSink& sink, uint32_t now);
    SdTransferState step(uint32_t now);
    uint64_t sent() const { return sent_; }
private:
    ISdStoredFile& source_; ISdDownloadSink& sink_;
    const uint64_t expected_;
    uint64_t sent_=0;
    uint32_t started_, progress_;
    uint8_t buffer_[1024];
    size_t count_=0, offset_=0;
};
class ISdDownloadJob {
public:
    virtual ~ISdDownloadJob() = default;
    virtual bool service(uint32_t now) = 0;
};
}
