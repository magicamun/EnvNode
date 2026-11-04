// EnvNode: see repository LICENSE.
#pragma once
#include <Arduino.h>
#include <utility>
namespace EnvNode {
class IHttpResponseSink {
public:
    virtual ~IHttpResponseSink() = default;
    virtual bool connected() = 0;
    // 0 means temporary backpressure, negative means a permanent socket error.
    virtual int send(const char* data,size_t size) = 0;
};
enum class HttpResponseState { Idle, More, Complete, Disconnected, Timeout, WriteFailed };
// Own the rendered page, not a second serialized copy. No allocations in step().
class HttpResponsePump {
public:
    static constexpr size_t MaxBytes=65536;
    static constexpr size_t StepBytes=8192;
    static constexpr unsigned StepCalls=8;
    static constexpr uint32_t StepTimeMs=2;
    bool begin(String&& wireHeader,String&& pageHeader,String&& content,const char* footer,uint32_t now) {
        if(active_ || !footer || wireHeader.isEmpty())return false;
        const size_t footerBytes=strlen(footer);
        const size_t sizes[]={wireHeader.length(),pageHeader.length(),content.length(),footerBytes};
        size_t total=0;
        for(size_t size:sizes) {if(size>MaxBytes-total)return false;total+=size;}
        parts_[0]=std::move(wireHeader);parts_[1]=std::move(pageHeader);parts_[2]=std::move(content);
        footer_=footer;footerBytes_=footerBytes;expected_=total;sent_=0;part_=offset_=0;
        started_=lastProgress_=now;active_=true;return true;
    }
    HttpResponseState step(IHttpResponseSink& sink,uint32_t now) {
        if(!active_)return HttpResponseState::Idle;
        if(!sink.connected())return finish(HttpResponseState::Disconnected);
        if(now-started_>=60000 || now-lastProgress_>=10000)return finish(HttpResponseState::Timeout);
        const uint32_t stepStarted=millis();
        size_t budget=StepBytes;
        for(unsigned calls=0;calls<StepCalls && budget;++calls) {
            while(part_<4 && offset_==length(part_)) {++part_;offset_=0;}
            if(part_==4)return finish(HttpResponseState::Complete);
            // Socket writes must remain nonblocking. Stop between calls even when
            // the byte budget has not been exhausted; no waiting for capacity.
            if(calls && static_cast<uint32_t>(millis()-stepStarted)>=StepTimeMs)
                return HttpResponseState::More;
            size_t size=length(part_)-offset_;if(size>1460)size=1460;if(size>budget)size=budget;
            const int written=sink.send(data(part_)+offset_,size);
            if(written<0 || static_cast<size_t>(written)>size)return finish(HttpResponseState::WriteFailed);
            if(!written)return HttpResponseState::More;
            offset_+=written;sent_+=written;budget-=written;lastProgress_=now;
        }
        return sent_==expected_?finish(HttpResponseState::Complete):HttpResponseState::More;
    }
    bool active() const {return active_;}
    size_t sent() const {return sent_;}
    size_t expected() const {return expected_;}
    uint32_t started() const {return started_;}
private:
    const char* data(size_t part) const {return part<3?parts_[part].c_str():footer_;}
    size_t length(size_t part) const {return part<3?parts_[part].length():footerBytes_;}
    HttpResponseState finish(HttpResponseState state) {
        active_=false;for(auto& part:parts_)part=String();footer_=nullptr;footerBytes_=0;return state;
    }
    String parts_[3];const char* footer_=nullptr;
    size_t footerBytes_=0,expected_=0,sent_=0,part_=0,offset_=0;
    uint32_t started_=0,lastProgress_=0;bool active_=false;
};
}
