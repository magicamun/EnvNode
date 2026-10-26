// EnvNode: see repository LICENSE.
#pragma once
#include "SdFileTransfer.h"
namespace EnvNode {
// Capacity guard for the first FAT32 formatter; no implicit format on mount.
bool supportedSdFormatCapacity(uint64_t bytes);
class SdFormatTicket {
public:
    bool issue(const char* token,const SdFileStamp& card,uint32_t now);
    bool consume(const char* token,const char* phrase,uint32_t now,SdFileStamp& card);
    void clear() { ticket_.clear(); }
private:
    SdDeleteTicket ticket_;
};
enum class SdFormatState { Running, Complete, Failed, MediaChanged };
class ISdFormatDevice {
public:
    virtual ~ISdFormatDevice() = default;
    virtual bool inspect(SdFileStamp& card) = 0;
    virtual bool format() = 0;
    virtual bool verify() = 0;
};
SdFormatState runSdCardFormat(ISdFormatDevice& device,const SdFileStamp& expected);
class ISdFormatJob {
public:
    virtual ~ISdFormatJob() = default;
    virtual SdFormatState state() const = 0;
    virtual int errorCode() const = 0;
};
}
