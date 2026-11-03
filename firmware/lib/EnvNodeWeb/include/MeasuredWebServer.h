// EnvNode: see repository LICENSE.
#pragma once
#include <WebServer.h>
namespace EnvNode {
// Observe SDK writes without changing its parser, response framing or timeouts.
class MeasuredWebServer : public WebServer {
public:
    explicit MeasuredWebServer(uint16_t port) : WebServer(port) {}
    void beginMeasurement() {
        started_=millis();firstWriteMs_=writeMs_=bytes_=0;wrote_=false;route_=String();
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
        beforeWrite();const uint32_t started=millis();
        const size_t written=WebServer::_currentClientWrite(data,size);
        writeMs_+=millis()-started;bytes_+=written;return written;
    }
    size_t _currentClientWrite_P(const char* data,size_t size) override {
        beforeWrite();const uint32_t started=millis();
        const size_t written=WebServer::_currentClientWrite_P(data,size);
        writeMs_+=millis()-started;bytes_+=written;return written;
    }
    uint32_t started_=0,firstWriteMs_=0,writeMs_=0;
    size_t bytes_=0;bool wrote_=false;String route_;
};
}
