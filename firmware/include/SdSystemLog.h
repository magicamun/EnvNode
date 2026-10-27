// EnvNode: see repository LICENSE.
#pragma once
#include "ILogEntrySink.h"
#include <memory>
namespace EnvNode {
constexpr size_t SdLogQueueEntries=28; // 7840 bytes on ESP32, below 8 KiB.
constexpr size_t SdLogPartBytes=1024*1024;
constexpr uint64_t SdLogBudgetBytes=32ULL*1024*1024;
// Caller serializes queue access. No filesystem calls or per-entry allocation.
class SdLogQueue {
public:
    SdLogQueue();
    bool push(const LogEntry& entry);
    bool pop(LogEntry& entry);
    uint32_t takeLost();
    size_t count() const {return count_;}
    uint32_t dropped() const {return dropped_;}
    bool available() const {return entries_!=nullptr;}
    void clear();
private:
    std::unique_ptr<LogEntry[]> entries_;
    size_t head_=0,count_=0;
    uint32_t lost_=0,dropped_=0;
};
size_t encodeSdLogEntry(const LogEntry& entry,char* output,size_t capacity);
bool managedSdLogName(const char* name);
class ISdLogDirectory {
public:
    virtual ~ISdLogDirectory()=default;
    virtual bool scan(uint64_t& total,char* oldest,size_t capacity,const char* protectedName)=0;
    virtual bool remove(const char* name)=0;
};
bool pruneSdSystemLogs(ISdLogDirectory& directory,size_t needed,const char* protectedName,uint64_t budget=SdLogBudgetBytes);
enum class SdLogWriteResult { Written, Busy, Failed };
class ISdLogStorage {
public:
    virtual ~ISdLogStorage()=default;
    // Entire batch accepted or retry in a new file after partial write/failure.
    virtual SdLogWriteResult append(const char* data,size_t size)=0;
};
// Worker-owned batch. Queue access is supplied by the platform's short critical section.
class SdLogBatch {
public:
    bool add(const LogEntry& entry,uint32_t now);
    bool addLoss(uint32_t count,uint32_t now);
    SdLogWriteResult flush(ISdLogStorage& storage,uint32_t now,bool force=false);
    bool due(uint32_t now) const;
    size_t bytes() const {return size_;}
    void clear();
private:
    char data_[4096];size_t size_=0;
    uint32_t first_=0,retryAt_=0;bool retry_=false;
};
}
