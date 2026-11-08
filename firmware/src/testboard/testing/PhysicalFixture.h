// EnvNode: see repository LICENSE.
#pragma once
#include "GuidedSelfTest.h"
#include <functional>
namespace EnvNode { namespace Testing {
class PhysicalFixture : public ITestFixture {
public:
    static constexpr uint16_t LowMaximum=200, ConnectedMinimum=800, HighMaximum=3800;
    struct Readings {uint16_t low=0, high=0;};
    PhysicalFixture(IDiagnosticIo& io,std::function<uint32_t()> clock):io_(io),clock_(clock) {}
    bool select(Pigtail pigtail,uint8_t contact,uint8_t input) override;
    Sample read(bool& connected) override;
    void release() override;
    bool trace(MeasurementTrace& value) const override {value=trace_;return true;}
    const Readings& readings() const {return readings_;}
private:
    IDiagnosticIo& io_;std::function<uint32_t()> clock_;
    DiagnosticPath path_{{0,0},-1,0,false};
    Readings readings_;
    MeasurementTrace trace_;
    uint32_t switched_=0,lastSample_=0,started_=0,sum_=0;
    uint16_t minimum_=4095,maximum_=0;
    uint8_t samples_=0;
    bool selected_=false;
};
class SelectableFixture : public ITestFixture {
public:
    SelectableFixture(ITestFixture& simulation,ITestFixture* physical=nullptr,bool simulated=true)
        :simulation_(simulation),physical_(physical),simulated_(simulated||!physical) {}
    bool simulated() const {return simulated_;}
    bool physicalAvailable() const {return physical_!=nullptr;}
    bool setSimulation(bool value) {
        if(!value&&!physical_)return false;
        release();simulated_=value;return true;
    }
    bool select(Pigtail p,uint8_t c,uint8_t i) override {return current().select(p,c,i);}
    Sample read(bool& connected) override {return current().read(connected);}
    void release() override {current().release();}
    bool trace(MeasurementTrace& value) const override {return current().trace(value);}
private:
    const ITestFixture& current() const {return simulated_?simulation_:*physical_;}
    ITestFixture& current(){return simulated_?simulation_:*physical_;}
    ITestFixture& simulation_;ITestFixture* physical_;bool simulated_;
};
} }
