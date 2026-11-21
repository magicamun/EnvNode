// EnvNode: see repository LICENSE.
#pragma once
#include <WebServer.h>
#include <utility>
#include "HttpClientGate.h"
namespace EnvNode {
// Observe SDK writes and prepare fixed-length headers for deferred delivery.
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
    void prepareDeferredHeader(String& header, int status, const char* type, size_t length) {
        setContentLength(length);
        _prepareHeader(header, status, type, length);
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
        const uint32_t started=millis();
        const size_t written=WebServer::_currentClientWrite(data,size);
        writeMs_+=millis()-started;bytes_+=written;return written;
    }
    size_t _currentClientWrite_P(const char* data,size_t size) override {
        beforeWrite();
        const uint32_t started=millis();
        const size_t written=WebServer::_currentClientWrite_P(data,size);
        writeMs_+=millis()-started;bytes_+=written;return written;
    }
#if defined(ARDUINO_ARCH_ESP32)
    HttpClientGate<WiFiClient> waitingClients_;
#endif
    uint32_t started_=0,firstWriteMs_=0,writeMs_=0;
    size_t bytes_=0;bool wrote_=false;String route_;
};
}
