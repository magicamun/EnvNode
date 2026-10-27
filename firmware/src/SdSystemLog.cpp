// EnvNode: see repository LICENSE.
#include "SdSystemLog.h"
#include <cstring>
#include <cstdio>
#include <new>
namespace EnvNode {
SdLogQueue::SdLogQueue() : entries_(new(std::nothrow) LogEntry[SdLogQueueEntries]) {}
bool SdLogQueue::push(const LogEntry& entry) {
    if(!entries_ || count_==SdLogQueueEntries) {++lost_;++dropped_;return false;}
    entries_[(head_+count_)%SdLogQueueEntries]=entry;++count_;return true;
}
bool SdLogQueue::pop(LogEntry& entry) {
    if(!count_) return false;
    entry=entries_[head_];head_=(head_+1)%SdLogQueueEntries;--count_;return true;
}
uint32_t SdLogQueue::takeLost() {const uint32_t result=lost_;lost_=0;return result;}
void SdLogQueue::clear() {head_=0;count_=0;lost_=0;}
size_t encodeSdLogEntry(const LogEntry& e,char* output,size_t capacity) {
    if(!output || capacity<2) return 0;
    const char* levels[]={"DEBUG","INFO","WARN","ERROR"};
    const unsigned level=static_cast<unsigned>(e.level);
    char epoch[32];
    if(e.wallClockValid) snprintf(epoch,sizeof(epoch),"%lld",static_cast<long long>(e.epochSeconds));
    else strcpy(epoch,"null");
    const int prefix=snprintf(output,capacity,"{\"type\":\"log\",\"sequence\":%lu,\"uptime_ms\":%lu,\"epoch_s\":%s,\"level\":\"%s\",\"message\":\"",
        static_cast<unsigned long>(e.sequence),static_cast<unsigned long>(e.monotonicMs),epoch,level<4 ? levels[level] : "UNKNOWN");
    if(prefix<0 || static_cast<size_t>(prefix)>=capacity) return 0;
    size_t pos=prefix;
    for(size_t i=0;i<LogMessageCapacity && e.message[i];++i) {
        const unsigned char c=e.message[i];
        char escaped[7];const char* value=escaped;size_t length;
        if(c=='"' || c=='\\') {escaped[0]='\\';escaped[1]=c;length=2;}
        else if(c<32 || c==127) {snprintf(escaped,sizeof(escaped),"\\u%04x",c);length=6;}
        else {escaped[0]=c;length=1;}
        if(pos+length+4>=capacity) return 0;
        memcpy(output+pos,value,length);pos+=length;
    }
    if(pos+4>capacity) return 0;
    memcpy(output+pos,"\"}\n",4);return pos+3;
}
bool managedSdLogName(const char* name) {
    if(!name || strlen(name)!=31 || strncmp(name,"boot-",5) || strncmp(name+15,"-part-",6) || strcmp(name+25,".jsonl")) return false;
    for(size_t i=5;i<15;++i) if(name[i]<'0' || name[i]>'9') return false;
    for(size_t i=21;i<25;++i) if(name[i]<'0' || name[i]>'9') return false;
    return true;
}
bool pruneSdSystemLogs(ISdLogDirectory& directory,size_t needed,const char* protectedName,uint64_t budget) {
    if(!budget || needed>budget || !protectedName) return false;
    for(unsigned pass=0;pass<64;++pass) {
        uint64_t total=0;char oldest[32]={};
        if(!directory.scan(total,oldest,sizeof(oldest),protectedName)) return false;
        if(total<=budget-needed) return true;
        if(!managedSdLogName(oldest) || !strcmp(oldest,protectedName) || !directory.remove(oldest)) return false;
    }
    return false;
}
bool SdLogBatch::add(const LogEntry& entry,uint32_t now) {
    char line[1800];const size_t length=encodeSdLogEntry(entry,line,sizeof(line));
    if(!length || length>sizeof(data_)-size_) return false;
    if(!size_) first_=now;
    memcpy(data_+size_,line,length);size_+=length;return true;
}
bool SdLogBatch::addLoss(uint32_t count,uint32_t now) {
    LogEntry entry;entry.monotonicMs=now;entry.level=LogLevel::Warn;
    snprintf(entry.message,sizeof(entry.message),"SD log buffer overflow: %lu entries dropped",static_cast<unsigned long>(count));
    return add(entry,now);
}
bool SdLogBatch::due(uint32_t now) const {return size_ && (size_>=3072 || now-first_>=5000);}
SdLogWriteResult SdLogBatch::flush(ISdLogStorage& storage,uint32_t now,bool force) {
    if(!size_ || (!force && !due(now)) || (retry_ && now-retryAt_<30000)) return SdLogWriteResult::Busy;
    const auto result=storage.append(data_,size_);
    if(result==SdLogWriteResult::Written) clear();
    else if(result==SdLogWriteResult::Failed) {retry_=true;retryAt_=now;}
    return result;
}
void SdLogBatch::clear() {size_=0;retry_=false;first_=0;}
}
