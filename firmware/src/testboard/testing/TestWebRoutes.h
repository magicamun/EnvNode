// EnvNode: see repository LICENSE.
#pragma once
#include "IWebRoutes.h"
#include "IWebPagePresenter.h"
#include "WebHost.h"
#include "TestRunner.h"
#include "SimulatedFixture.h"
#include "../profiles/ProfileRepository.h"
namespace EnvNode { namespace Testing {
class TestWebRoutes : public IWebRoutes {
public:
    TestWebRoutes(WebHost& host, IWebPagePresenter& pages, TestRunner& runner, SimulatedFixture& fixture);
    void attachProfiles(ProfileRepository& profiles) { profiles_ = &profiles; }
    void registerRoutes(WebServer& server) override;
    void tick(uint32_t now) { runner_.tick(now); }
    const TestRunner& runner() const { return runner_; }
private:
    void page();
    void state();
    void action();
    WebHost& host_;
    IWebPagePresenter& pages_;
    SimulatedFixture& fixture_;
    TestRunner& runner_;
    ProfileRepository* profiles_ = nullptr;
};
} }
