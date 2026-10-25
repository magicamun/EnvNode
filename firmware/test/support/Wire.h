// EnvNode native Wire double: see repository LICENSE.
#pragma once
#include "Arduino.h"
#include <algorithm>
#include <deque>
#include <map>
#include <vector>
class TwoWire {
public:
    struct Transmission { uint8_t address; std::vector<uint8_t> bytes; bool stop; };
    bool beginOk = true;
    int begins = 0, sda = -1, scl = -1, requests = 0;
    uint8_t device = 0, defaultResult = 0;
    size_t writeLimit = 2048, readLimit = 2048, cursor = 0;
    std::map<uint8_t, uint8_t> results;
    std::map<uint8_t, std::vector<uint8_t>> memory;
    std::deque<uint8_t> endResults, incoming;
    std::vector<uint8_t> outgoing;
    std::vector<Transmission> transmissions;
    bool begin(int data, int clock) { ++begins; sda=data; scl=clock; return beginOk; }
    void beginTransmission(uint8_t address) { device=address; outgoing.clear(); }
    size_t write(uint8_t byte) { return write(&byte, 1); }
    size_t write(const uint8_t* bytes, size_t size) {
        const size_t count=std::min(size,writeLimit);
        outgoing.insert(outgoing.end(),bytes,bytes+count); return count;
    }
    uint8_t endTransmission(bool stop=true) {
        transmissions.push_back({device,outgoing,stop});
        uint8_t result=results.count(device) ? results[device] : defaultResult;
        if (!endResults.empty()) { result=endResults.front();endResults.pop_front(); }
        if (result!=0) return result;
        if (outgoing.size()>=2) {
            cursor=(static_cast<size_t>(outgoing[0])<<8)|outgoing[1];
            auto& data=storage(device);
            const size_t page=cursor-cursor%32;
            for(size_t i=2;i<outgoing.size();++i) data[(page+(cursor%32+i-2)%32)%4096]=outgoing[i];
        }
        return 0;
    }
    size_t requestFrom(uint8_t address,uint8_t size) {
        ++requests; incoming.clear(); const size_t count=std::min(static_cast<size_t>(size),readLimit);
        auto& data=storage(address);
        for(size_t i=0;i<count;++i) incoming.push_back(data[(cursor+i)%4096]);
        cursor+=count; return count;
    }
    int available() {return static_cast<int>(incoming.size());}
    int read() { if(incoming.empty())return -1;int byte=incoming.front();incoming.pop_front();return byte; }
    std::vector<uint8_t>& storage(uint8_t address) {
        auto& data=memory[address];if(data.empty())data.resize(4096,0xff);return data;
    }
};
inline TwoWire& nativeWire0() { static TwoWire instance; return instance; }
inline TwoWire& nativeWire1() { static TwoWire instance; return instance; }
#define Wire nativeWire0()
#define Wire1 nativeWire1()
