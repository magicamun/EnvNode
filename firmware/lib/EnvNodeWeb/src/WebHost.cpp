// EnvNode: see repository LICENSE.
#include "WebHost.h"
#include "WebPageFrame.h"
#include "WebStyle.h"
#include <cstring>
#if defined(ARDUINO_ARCH_ESP32)
#include <sys/socket.h>
#include <cerrno>
#endif
namespace EnvNode {
WebHost::WebHost(ILogger& logger, IWiFiService& wifi, uint16_t port)
    : logger_(logger), wifi_(wifi), server_(port) {}
WebServer& WebHost::server() { return server_; }
void WebHost::registerRoutes(IWebRoutes& routes) { routes.registerRoutes(server_); }
void WebHost::begin() {
    server_.on("/style.css", HTTP_GET, [this]() {
        server_.sendHeader("Cache-Control", "public, max-age=86400");
        queuePage(String(),String(),200,"text/css",sharedWebStyle());
    });
    server_.begin();
    logger_.info("Web administration started");
}
void WebHost::sendStaticContent(const char* type,const char* bytes) {
    if(!administrationAvailable() || !bytes) {
        server_.send(503,"text/plain","Static content unavailable");return;
    }
    server_.sendHeader("Cache-Control","no-cache");
    queuePage(String(),String(),200,type,bytes);
}
void WebHost::afterResponse(std::function<void()> action) {
    if(!response_.active()) {action();return;}
    afterResponse_=std::move(action);
}
void WebHost::loop() {
    if(afterResponse_ && !response_.active()) {
        if(millis()-afterResponseAt_>=200) {auto action=std::move(afterResponse_);afterResponse_=nullptr;action();}
        return;
    }
    if(response_.active()) {serviceResponse();return;}
    server_.beginMeasurement();headerRenderMs_=0;
    const uint32_t started=millis();
    server_.handleClient();
    const uint32_t elapsed=millis()-started;
    if(elapsed>=1000) logger_.warnf(
        "Web request slow: elapsed_ms=%lu route=%s first_write_ms=%lu send_ms=%lu header_ms=%lu bytes=%u heap=%u largest=%u",
        static_cast<unsigned long>(elapsed),server_.responseRoute().isEmpty() ? "(no response)" : server_.responseRoute().c_str(),
        static_cast<unsigned long>(server_.firstWriteMs()),static_cast<unsigned long>(server_.writeMs()),
        static_cast<unsigned long>(headerRenderMs_),static_cast<unsigned>(server_.writtenBytes()),
        static_cast<unsigned>(ESP.getFreeHeap()),static_cast<unsigned>(ESP.getMaxAllocHeap()));
    if(response_.active())serviceResponse();
}
bool WebHost::administrationAvailable() const {
    return wifi_.inSetupAccessPointMode() || wifi_.connected();
}
void WebHost::sendPage(const std::function<String()>& renderHeader,
    String content, int status, const char* unavailableMessage,
    const char* renderFailureMessage) {
    if (!administrationAvailable()) {
        server_.send(503, "text/plain", unavailableMessage);
        return;
    }
    const uint32_t started=millis();
    String header = renderHeader();
    headerRenderMs_+=millis()-started;
    const char* footer = webPageFooter();
    if (header.isEmpty() || content.isEmpty()) {
        logger_.errorf("Web page rendering failed: header=%u content=%u heap=%u largest=%u",
            static_cast<unsigned>(header.length()), static_cast<unsigned>(content.length()),
            static_cast<unsigned>(ESP.getFreeHeap()), static_cast<unsigned>(ESP.getMaxAllocHeap()));
        server_.send(503, "text/plain", renderFailureMessage);
        return;
    }
    queuePage(std::move(header),std::move(content),status,"text/html; charset=utf-8",footer);
}
void WebHost::queuePage(String header,String content,int status,const char* type,const char* footer) {
    if(response_.active()) {logger_.error("HTTP response already active");return;}
    responseClient_=server_.client();responseRoute_=server_.uri();
    responseSteps_=responseBlocked_=responseMaxGapMs_=responseMaxStepMs_=0;
    responseLastStepMs_=millis();
    server_.setContentLength(header.length()+content.length()+strlen(footer));
    server_.captureResponseHeaders();
    server_.send(status,type,String());
    String wireHeader;
    if(!server_.finishHeaderCapture(wireHeader)
        || !response_.begin(std::move(wireHeader),std::move(header),std::move(content),footer,millis())) {
        logger_.warnf("HTTP response aborted: route=%s reason=buffer_limit_or_allocation",responseRoute_.c_str());
        responseClient_.stop();
    }
}
bool WebHost::connected() {return responseClient_.connected();}
int WebHost::send(const char* data,size_t size) {
#if defined(ARDUINO_ARCH_ESP32)
    const int result=::send(responseClient_.fd(),data,size,MSG_DONTWAIT);
    if(result<0 && (errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR))return 0;
    return result;
#else
    return responseClient_.sendNonblocking(data,size);
#endif
}
void WebHost::serviceResponse() {
    const uint32_t now=millis();
    const uint32_t gap=now-responseLastStepMs_;
    if(gap>responseMaxGapMs_)responseMaxGapMs_=gap;
    responseLastStepMs_=now;
    const size_t before=response_.sent();
    const auto state=response_.step(*this,now);
    const uint32_t stepMs=millis()-now;
    if(stepMs>responseMaxStepMs_)responseMaxStepMs_=stepMs;
    ++responseSteps_;
    if(state==HttpResponseState::More && response_.sent()==before)++responseBlocked_;
    if(state==HttpResponseState::More)return;
    const uint32_t elapsed=now-response_.started();
    if(state!=HttpResponseState::Complete || elapsed>=1000) {
        char message[256];
        snprintf(message,sizeof(message),
            "HTTP response %s: route=%.40s elapsed_ms=%lu sent=%u expected=%u state=%u steps=%lu blocked_steps=%lu max_gap_ms=%lu max_step_ms=%lu heap=%u largest=%u",
            state==HttpResponseState::Complete?"complete":"aborted",responseRoute_.c_str(),
            static_cast<unsigned long>(elapsed),static_cast<unsigned>(response_.sent()),
            static_cast<unsigned>(response_.expected()),static_cast<unsigned>(state),
            static_cast<unsigned long>(responseSteps_),static_cast<unsigned long>(responseBlocked_),
            static_cast<unsigned long>(responseMaxGapMs_),static_cast<unsigned long>(responseMaxStepMs_),
            static_cast<unsigned>(ESP.getFreeHeap()),static_cast<unsigned>(ESP.getMaxAllocHeap()));
        logger_.log(state==HttpResponseState::Complete?LogLevel::Info:LogLevel::Warn,message);
    }
    responseClient_.stop();responseRoute_=String();afterResponseAt_=now;
}
}
