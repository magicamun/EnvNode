// EnvNode native SNTP double: see repository LICENSE.
#pragma once
#include <string>
#include <vector>
struct NativeSntpState {
    bool enabled=false;
    int stops=0,starts=0,mode=-1;
    std::string servers[3];
    std::vector<std::string> events;
};
inline NativeSntpState& nativeSntp() {static NativeSntpState state;return state;}
constexpr int ESP_SNTP_OPMODE_POLL=0;
inline bool esp_sntp_enabled(){return nativeSntp().enabled;}
inline void esp_sntp_stop(){auto& s=nativeSntp();++s.stops;s.enabled=false;s.events.push_back("stop");}
inline void esp_sntp_setoperatingmode(int mode){nativeSntp().mode=mode;nativeSntp().events.push_back("mode");}
inline void esp_sntp_setservername(int index,const char* name){nativeSntp().servers[index]=name?name:"";nativeSntp().events.push_back("server");}
inline void esp_sntp_init(){auto& s=nativeSntp();++s.starts;s.enabled=true;s.events.push_back("start");}
