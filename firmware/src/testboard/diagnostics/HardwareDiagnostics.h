// EnvNode: see repository LICENSE.
#pragma once
#include "IWebRoutes.h"
#include "IWebPagePresenter.h"
#include "WebHost.h"
#include "../testing/FixtureRouting.h"
#include <functional>
#include "../testing/GuidedSelfTest.h"
namespace EnvNode {
class HardwareDiagnostics : public IWebRoutes, private Testing::IDiagnosticIo {
public:
    HardwareDiagnostics(WebHost& host, IWebPagePresenter& pages, std::function<bool()> available, Testing::IDiagnosticIo& io,Testing::FixtureSelection* selection=nullptr)
        : host_(host), pages_(pages), available_(available), io_(io), selfTest_(*this,selection),selection_(selection) {}
    void registerRoutes(WebServer& server) override;
    void tick(uint32_t now);
    void resetFixture() {stop();selfTest_.clear();selfSelected_=0;++selfGeneration_;}
    bool active() const { return active_ || selfTest_.running(); }
private:
    void stop();
    void releaseDiagnostic() override;
    void selectDiagnostic(const Testing::DiagnosticPath& path) override;
    uint16_t sampleDiagnostic() override;
    void selfTestPage();
    void selfTestState();
    void selfTestAction();
    void apply();
    void page();
    void state();
    WebHost& host_;
    IWebPagePresenter& pages_;
    std::function<bool()> available_;
    Testing::IDiagnosticIo& io_;
    Testing::GuidedSelfTest selfTest_;
    Testing::FixtureSelection* selection_;
    size_t selfSelected_ = 0;
    uint32_t selfGeneration_ = 0;
    bool active_ = false;
    uint32_t lastCommand_ = 0;
    int output_ = -1, channel_ = 0, level_ = 0, input_ = -1, inputChannel_ = 0;
};
}
