// EnvNode: see repository LICENSE.
#pragma once
#include "MeasuredWebServer.h"
#include "HttpResponsePump.h"
#include <type_traits>
#include <utility>
#include <functional>
#include "IWebRoutes.h"
#include "IWiFiService.h"
#include "Logger.h"
namespace EnvNode {
class WebHost : private IHttpResponseSink {
public:
    WebHost(ILogger& logger, IWiFiService& wifi, uint16_t port = 80);
    WebServer& server();
    void registerRoutes(IWebRoutes& routes);
    void begin();
    void loop();
    void afterResponse(std::function<void()> action);
    bool responsePending() const {return response_.active() || static_cast<bool>(afterResponse_);}
    bool administrationAvailable() const;
    void sendPage(const std::function<String()>& renderHeader, String content,
        int status, const char* unavailableMessage, const char* renderFailureMessage);
private:
    bool connected() override;
    int send(const char* data,size_t size) override;
    void serviceResponse();
    void queuePage(String header,String content,int status,const char* type,const char* footer);
    using Client=typename std::decay<decltype(std::declval<WebServer&>().client())>::type;
    Client responseClient_;
    HttpResponsePump response_;
    std::function<void()> afterResponse_;
    uint32_t afterResponseAt_=0;
    String responseRoute_;
    uint32_t responseSteps_=0,responseBlocked_=0,responseLastStepMs_=0;
    uint32_t responseMaxGapMs_=0,responseMaxStepMs_=0;
    ILogger& logger_;
    IWiFiService& wifi_;
    MeasuredWebServer server_;
    uint32_t headerRenderMs_=0;
};
}
