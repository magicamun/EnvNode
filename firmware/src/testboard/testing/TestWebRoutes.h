// EnvNode: see repository LICENSE.
#pragma once
#include "IWebRoutes.h"
#include "IWebPagePresenter.h"
#include "WebHost.h"
#include "TestRunner.h"
#include "SimulatedFixture.h"
#include "PhysicalFixture.h"
#include "../profiles/ProfileRepository.h"
namespace EnvNode { namespace Testing {
class TestWebRoutes : public IWebRoutes {
public:
    TestWebRoutes(WebHost& host, IWebPagePresenter& pages, TestRunner& runner, SimulatedFixture& fixture);
    void attachSelection(FixtureSelection& selection,std::function<void()> changed={},std::function<bool(unsigned)> persist={}) {selection_=&selection;selectionChanged_=changed;persistSelection_=persist;}
    void setBuildIdentity(const char* commit,const char* build) {gitCommit_=commit;buildNumber_=build;}
    void attachMeasurement(SelectableFixture& fixture) { measurement_ = &fixture; }
    void attachProfiles(ProfileRepository& profiles) { profiles_ = &profiles; }
    void setHardwareBusy(std::function<bool()> busy) { hardwareBusy_ = busy; }
    void registerRoutes(WebServer& server) override;
    void tick(uint32_t now) { runner_.tick(now); }
    const TestRunner& runner() const { return runner_; }
private:
    void page();
    void state();
    void action();
    void reportContact();
    FixtureSelection* selection_=nullptr;
    std::function<void()> selectionChanged_;
    std::function<bool(unsigned)> persistSelection_;
    const char* gitCommit_="";
    const char* buildNumber_="";
    WebHost& host_;
    IWebPagePresenter& pages_;
    SimulatedFixture& fixture_;
    TestRunner& runner_;
    std::function<bool()> hardwareBusy_;
    SelectableFixture* measurement_ = nullptr;
    ProfileRepository* profiles_ = nullptr;
};
} }
