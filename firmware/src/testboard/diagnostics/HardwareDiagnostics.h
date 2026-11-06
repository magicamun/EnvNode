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
    HardwareDiagnostics(WebHost& host, IWebPagePresenter& pages, std::function<bool()> available)
        : host_(host), pages_(pages), available_(available), selfTest_(*this) {}
    void registerRoutes(WebServer& server) override;
    void tick(uint32_t now);
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
    Testing::GuidedSelfTest selfTest_;
    size_t selfSelected_ = 0;
    uint32_t selfGeneration_ = 0;
    bool active_ = false;
    uint32_t lastCommand_ = 0;
    int output_ = -1, channel_ = 0, level_ = 0, input_ = -1, inputChannel_ = 0;
};
}
