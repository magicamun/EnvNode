// EnvNode: see repository LICENSE.
#pragma once
#include <WebServer.h>
#include <utility>
#include "HttpClientGate.h"
namespace EnvNode {
// Observe SDK writes; capture fixed-length page headers for deferred delivery.
class MeasuredWebServer : public WebServer {
public:
    explicit MeasuredWebServer(uint16_t port) : WebServer(port) {}
    void handleClient() {
#if defined(ARDUINO_ARCH_ESP32)
        if(_currentStatus==HC_NONE) {
            WiFiClient ready;
            if(!waitingClients_.poll(_server,ready,millis())) {
                if(_nullDelay)delay(1);
                return;
            }
            _currentClient=std::move(ready);
            _currentStatus=HC_WAIT_READ;
            _statusChange=millis();
        }
#endif
        WebServer::handleClient();
    }
    void beginMeasurement() {
        started_=millis();firstWriteMs_=writeMs_=bytes_=0;wrote_=false;route_=String();
    }
    void captureResponseHeaders() {captured_=String();capture_=true;captureFailed_=false;}
    bool finishHeaderCapture(String& result) {
        capture_=false;result=std::move(captured_);return !captureFailed_;
    }
    uint32_t firstWriteMs() const {return firstWriteMs_;}
    uint32_t writeMs() const {return writeMs_;}
    size_t writtenBytes() const {return bytes_;}
    const String& responseRoute() const {return route_;}
private:
    void beforeWrite() {
        if(!wrote_) {wrote_=true;firstWriteMs_=millis()-started_;route_=uri();}
    }
    size_t _currentClientWrite(const char* data,size_t size) override {
        beforeWrite();
        if(capture_) {
            if(size>2048-captured_.length() || !captured_.concat(data,size))captureFailed_=true;
            return captureFailed_?0:size;
        }
        const uint32_t started=millis();
        const size_t written=WebServer::_currentClientWrite(data,size);
        writeMs_+=millis()-started;bytes_+=written;return written;
    }
    size_t _currentClientWrite_P(const char* data,size_t size) override {
        beforeWrite();
        if(capture_) {
            if(size>2048-captured_.length() || !captured_.concat(data,size))captureFailed_=true;
            return captureFailed_?0:size;
        }
        const uint32_t started=millis();
        const size_t written=WebServer::_currentClientWrite_P(data,size);
        writeMs_+=millis()-started;bytes_+=written;return written;
    }
#if defined(ARDUINO_ARCH_ESP32)
    HttpClientGate<WiFiClient> waitingClients_;
#endif
    String captured_;bool capture_=false,captureFailed_=false;
    uint32_t started_=0,firstWriteMs_=0,writeMs_=0;
    size_t bytes_=0;bool wrote_=false;String route_;
};
}
