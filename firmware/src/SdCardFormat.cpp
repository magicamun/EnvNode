// EnvNode: see repository LICENSE.
#include "SdCardFormat.h"
#include <cstring>
namespace EnvNode {
bool supportedSdFormatCapacity(uint64_t bytes) {
    return bytes>=3500000000ULL && bytes<=34359738368ULL;
}
SdFormatState runSdCardFormat(ISdFormatDevice& device,const SdFileStamp& expected) {
    SdFileStamp current;
    if(!supportedSdFormatCapacity(expected.bytes) || !device.inspect(current)) return SdFormatState::Failed;
    if(!sameSdFileStamp(expected,current)) return SdFormatState::MediaChanged;
    if(!device.format() || !device.verify()) return SdFormatState::Failed;
    return SdFormatState::Complete;
}
bool SdFormatTicket::issue(const char* token,const SdFileStamp& card,uint32_t now) {
    clear();
    return supportedSdFormatCapacity(card.bytes) && ticket_.issue("/format-card",token,card,now);
}
bool SdFormatTicket::consume(const char* token,const char* phrase,uint32_t now,SdFileStamp& card) {
    SdFileStamp expected;
    const bool valid=ticket_.consume("/format-card",token,now,expected);
    if(!valid || !phrase || strcmp(phrase,"FORMATIEREN")) return false;
    card=expected;return true;
}
}
