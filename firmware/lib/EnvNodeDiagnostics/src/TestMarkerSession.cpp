// EnvNode: see repository LICENSE.
#include "TestMarkerSession.h"
#include <cstring>
namespace EnvNode {
namespace {
bool visible(const char* text) {
    if(!text)return false;
    for(size_t i=0;i<DiagnosticTextBytes && text[i];++i)
        if(static_cast<unsigned char>(text[i])>32 && text[i]!=127)return true;
    return false;
}
TestMarkerResult outcome(DiagnosticRecordResult result) {
    switch(result) {
        case DiagnosticRecordResult::Accepted:return TestMarkerResult::Accepted;
        case DiagnosticRecordResult::Disabled:return TestMarkerResult::Disabled;
        case DiagnosticRecordResult::Unavailable:return TestMarkerResult::Unavailable;
        case DiagnosticRecordResult::Full:return TestMarkerResult::Full;
        case DiagnosticRecordResult::Busy:return TestMarkerResult::Busy;
        default:return TestMarkerResult::Invalid;
    }
}
}
TestMarkerResult TestMarkerSession::submit(TestMarkerAction action,const char* label,
    const char* note,uint32_t expectedActiveId,ITestMarkerRecorder& recorder) {
    if(action!=TestMarkerAction::Begin && action!=TestMarkerAction::Note && action!=TestMarkerAction::End)
        return TestMarkerResult::Invalid;
    if(expectedActiveId!=activeId())return TestMarkerResult::Conflict;
    if((action==TestMarkerAction::Begin && active_) || (action==TestMarkerAction::End && !active_))
        return TestMarkerResult::Conflict;
    if(action==TestMarkerAction::Begin && lastId_==UINT32_MAX)return TestMarkerResult::Exhausted;
    const char* selectedLabel=active_?label_:label;
    if(!selectedLabel || !note)return TestMarkerResult::Invalid;
    if(action==TestMarkerAction::Begin && !visible(selectedLabel))return TestMarkerResult::Invalid;
    if(action==TestMarkerAction::Note && !visible(selectedLabel) && !visible(note))return TestMarkerResult::Invalid;
    const uint32_t id=action==TestMarkerAction::Begin?lastId_+1:activeId();
    DiagnosticEvent event("test_marker");
    event.text("phase",action==TestMarkerAction::Begin?"begin":action==TestMarkerAction::End?"end":"note");
    event.integer("test_id",id);event.text("label",selectedLabel);event.text("text",note);event.text("origin","web");
    if(!event.valid())return TestMarkerResult::Invalid;
    const auto result=outcome(recorder.recordMarker(event));
    if(result==TestMarkerResult::Accepted) {
        if(action==TestMarkerAction::Begin) {lastId_=id;memmove(label_,selectedLabel,strlen(selectedLabel)+1);active_=true;}
        else if(action==TestMarkerAction::End)active_=false;
        else if(!active_ && visible(selectedLabel))memmove(label_,selectedLabel,strlen(selectedLabel)+1);
    }
    return result;
}
}
