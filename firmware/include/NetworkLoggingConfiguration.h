// EnvNode-owned: see repository LICENSE.
#pragma once
#include <Arduino.h>
#include <cstdio>
#include <cstring>
namespace EnvNode {
struct NetworkLoggingConfiguration {
    bool enabled=false,systemLog=false,systemDiagnostics=false;
    String address;
    uint16_t port=514;
};
inline bool validLoggingAddress(const String& value) {
    if(value.isEmpty()||value.length()>15)return false;
    for(size_t i=0;i<value.length();++i)if((value[i]<'0'||value[i]>'9')&&value[i]!='.')return false;
    unsigned octets[4];int end=0;
    if(sscanf(value.c_str(),"%u.%u.%u.%u%n",&octets[0],&octets[1],&octets[2],&octets[3],&end)!=4 || static_cast<size_t>(end)!=value.length())return false;
    for(unsigned n:octets)if(n>255)return false;
    char canonical[16];snprintf(canonical,sizeof(canonical),"%u.%u.%u.%u",octets[0],octets[1],octets[2],octets[3]);
    if(strcmp(canonical,value.c_str()))return false;
    // Unicast numeric IPv4 only. No broadcast/multicast or zero destination.
    return octets[0]>0&&octets[0]<224&&!(octets[0]==255&&octets[1]==255&&octets[2]==255&&octets[3]==255);
}
inline bool validNetworkLogging(const NetworkLoggingConfiguration& c) {
    return c.port>0&&(c.address.isEmpty()?!c.enabled:validLoggingAddress(c.address));
}
inline String encodeNetworkLogging(const NetworkLoggingConfiguration& c) {
    return String(c.enabled?1:0)+"|"+String(c.port)+"|"+String(c.systemLog?1:0)+"|"+String(c.systemDiagnostics?1:0)+"|"+c.address;
}
inline bool decodeNetworkLogging(const String& text,NetworkLoggingConfiguration& out) {
    unsigned enabled,port,system,diagnostic;int pos=0;
    if(sscanf(text.c_str(),"%u|%u|%u|%u|%n",&enabled,&port,&system,&diagnostic,&pos)!=4||pos==0||enabled>1||system>1||diagnostic>1||port==0||port>65535)return false;
    NetworkLoggingConfiguration c;c.enabled=enabled;c.port=port;c.systemLog=system;c.systemDiagnostics=diagnostic;c.address=text.c_str()+pos;
    if(!validNetworkLogging(c))return false;out=c;return true;
}
}
