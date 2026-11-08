// EnvNode: see repository LICENSE.
#include "PhysicalFixture.h"
namespace EnvNode { namespace Testing {
bool PhysicalFixture::select(Pigtail p,uint8_t contact,uint8_t input) {
    release();trace_=MeasurementTrace();trace_.reason=MeasurementReason::SelectionFailed;MuxAddress source{},destination{};
    if(!resolveFixturePath(selection_?selection_->definition():fixtureRevision01(),p,contact,input,source,destination))return false;
    path_={source,int(destination.device),destination.channel,false};
    trace_.reason=MeasurementReason::None;readings_=Readings();sum_=samples_=0;minimum_=4095;maximum_=0;
    switched_=lastSample_=started_=clock_();selected_=true;io_.selectDiagnostic(path_);return true;
}
ITestFixture::Sample PhysicalFixture::read(bool& connected) {
    connected=false;if(!selected_)return Sample::Error;
    const uint32_t now=clock_();
    if(uint32_t(now-started_)>=500){trace_.reason=MeasurementReason::Timeout;release();return Sample::Error;}
    const uint32_t settle=100;
    if(uint32_t(now-switched_)<settle||(samples_&&uint32_t(now-lastSample_)<2))return Sample::Busy;
    const uint16_t sample=io_.sampleDiagnostic();lastSample_=now;sum_+=sample;
    if(path_.high)trace_.high[trace_.highCount++]=sample;
    else trace_.low[trace_.lowCount++]=sample;
    if(sample<minimum_)minimum_=sample;if(sample>maximum_)maximum_=sample;
    if(++samples_<8)return Sample::Busy;
    if(!path_.high) {
        readings_.low=uint16_t(sum_/8);
        if(maximum_>LowMaximum){trace_.reason=MeasurementReason::LowNotLow;release();return Sample::Error;}
        path_.high=true;io_.selectDiagnostic(path_);switched_=lastSample_=now;
        sum_=samples_=0;minimum_=4095;maximum_=0;return Sample::Busy;
    }
    readings_.high=uint16_t(sum_/8);
    const bool isolated=maximum_<=LowMaximum;
    connected=minimum_>=ConnectedMinimum&&maximum_<=HighMaximum;
    if(!isolated&&!connected)trace_.reason=MeasurementReason::HighAmbiguous;
    release();return isolated||connected?Sample::Ready:Sample::Error;
}
void PhysicalFixture::release(){if(selected_)io_.releaseDiagnostic();selected_=false;}
} }
