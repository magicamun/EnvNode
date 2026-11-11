// EnvNode: see repository LICENSE.
#pragma once
#include "IDiagnosticRecorder.h"
namespace EnvNode {
enum class TestMarkerAction {Begin,Note,End};
enum class TestMarkerResult {Accepted,Invalid,Conflict,Exhausted,Disabled,Unavailable,Full,Busy};
class ITestMarkerRecorder {
public:
    virtual ~ITestMarkerRecorder()=default;
    virtual DiagnosticRecordResult recordMarker(const DiagnosticEvent&)=0;
};
// One optional operator-labelled test per boot. Main-loop use only.
// No NVS writes or dynamic storage.
class TestMarkerSession {
public:
    TestMarkerResult submit(TestMarkerAction,const char* label,const char* note,
        uint32_t expectedActiveId,ITestMarkerRecorder&);
    bool active() const {return active_;}
    uint32_t activeId() const {return active_?lastId_:0;}
    const char* label() const {return label_;}
private:
    uint32_t lastId_=0;bool active_=false;char label_[DiagnosticTextBytes]={};
};
static_assert(sizeof(TestMarkerSession)<=48,"bounded marker session RAM budget");
}
