// EnvNode: see repository LICENSE.
#pragma once
#include <cstddef>
#include <cstdint>
#include <utility>
namespace EnvNode {
// Wait for the first request byte without monopolizing the HTTP parser.
// Own only socket handles; do not read, copy or buffer request data.
template<class Client, size_t Capacity=2>
class HttpClientGate {
public:
    static_assert(Capacity>0,"HTTP waiting-client capacity");
    static constexpr uint32_t FirstByteWaitMs=5000;
    template<class Acceptor>
    bool poll(Acceptor& server,Client& selected,uint32_t now) {
        for(auto& slot:slots_) {
            if(!slot.used)continue;
            if(!slot.client.connected())release(slot);
            else if(!slot.client.available() && static_cast<uint32_t>(now-slot.since)>=FirstByteWaitMs) {
                release(slot);++expired_;
            }
        }
        bool ready=false;
        for(size_t n=0;n<Capacity;++n) {
            const size_t i=(next_+n)%Capacity;
            auto& slot=slots_[i];
            if(slot.used && slot.client.available()) {
                selected=std::move(slot.client);slot.client=Client();slot.used=false;
                next_=(i+1)%Capacity;ready=true;break;
            }
        }
        // One nonblocking accept per turn, independent of how many idle sockets
        // are waiting. A full pool of idle clients cannot starve a new request.
        Client incoming=server.available();
        if(!incoming.connected())return ready;
        if(!ready && incoming.available()) {selected=std::move(incoming);return true;}
        Slot* target=nullptr;
        for(auto& slot:slots_)if(!slot.used) {target=&slot;break;}
        if(!target) {
            uint32_t oldest=0;
            for(auto& slot:slots_)if(!slot.client.available()) {
                const uint32_t age=now-slot.since;
                if(!target || age>oldest) {target=&slot;oldest=age;}
            }
            if(target) {release(*target);++evicted_;}
        }
        if(!target) {incoming.stop();++rejected_;return ready;}
        target->client=std::move(incoming);target->since=now;target->used=true;
        return ready;
    }
    uint32_t expired() const {return expired_;}
    uint32_t evicted() const {return evicted_;}
    uint32_t rejected() const {return rejected_;}
private:
    struct Slot {Client client;uint32_t since=0;bool used=false;};
    static void release(Slot& slot) {slot.client.stop();slot.client=Client();slot.used=false;}
    Slot slots_[Capacity];size_t next_=0;
    uint32_t expired_=0,evicted_=0,rejected_=0;
};
}
