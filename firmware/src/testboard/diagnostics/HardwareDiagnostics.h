// EnvNode: see repository LICENSE.
#pragma once
#include "IWebRoutes.h"
#include "IWebPagePresenter.h"
#include "WebHost.h"
#include "../testing/FixtureRouting.h"
#include <functional>
namespace EnvNode {
class HardwareDiagnostics : public IWebRoutes {
public:
    HardwareDiagnostics(WebHost& host, IWebPagePresenter& pages, std::function<bool()> available)
        : host_(host), pages_(pages), available_(available) {}
    void registerRoutes(WebServer& server) override;
    void tick(uint32_t now);
    bool active() const { return active_; }
private:
    void stop();
    void apply();
    void page();
    void state();
    WebHost& host_;
    IWebPagePresenter& pages_;
    std::function<bool()> available_;
    bool active_ = false;
    uint32_t lastCommand_ = 0;
    int output_ = -1, channel_ = 0, level_ = 0, input_ = -1, inputChannel_ = 0;
};
}
