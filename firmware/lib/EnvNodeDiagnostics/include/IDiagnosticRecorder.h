// EnvNode: see repository LICENSE.
#pragma once
#include <cstddef>
#include <cstdint>
namespace EnvNode {
enum class DiagnosticSourceKind : uint8_t {System,Sensor,Actuator,Controller,Module,Count};
struct DiagnosticSource {
    constexpr DiagnosticSource(DiagnosticSourceKind category=DiagnosticSourceKind::System,uint16_t identifier=0):kind(category),id(identifier){}
    DiagnosticSourceKind kind;uint16_t id;
};
constexpr size_t DiagnosticMaxFields=6,DiagnosticNameBytes=32,DiagnosticKeyBytes=24,DiagnosticTextBytes=33;
enum class DiagnosticValueKind : uint8_t {Number,Integer,Boolean,Text};
struct DiagnosticField {
    char key[DiagnosticKeyBytes]={};DiagnosticValueKind kind=DiagnosticValueKind::Number;
    double number=0;int64_t integer=0;bool boolean=false;char text[DiagnosticTextBytes]={};
};
class DiagnosticEvent {
public:
    explicit DiagnosticEvent(const char* name);
    bool number(const char* key,double value);
    bool integer(const char* key,int64_t value);
    bool boolean(const char* key,bool value);
    bool text(const char* key,const char* value);
    bool valid() const {return valid_;}
    const char* name() const {return name_;}
    size_t count() const {return count_;}
    const DiagnosticField& field(size_t index) const {return fields_[index];}
private:
    DiagnosticField* add(const char* key,DiagnosticValueKind kind);
    char name_[DiagnosticNameBytes]={};DiagnosticField fields_[DiagnosticMaxFields];size_t count_=0;bool valid_=true;
};
enum class DiagnosticRecordResult {Accepted,Disabled,Unavailable,Invalid,Full,Busy};
class IDiagnosticRecorder {
public:
    virtual ~IDiagnosticRecorder()=default;
    virtual bool enabled(DiagnosticSource source) const=0;
    // Bounded RAM operation. Accepted does not promise durable storage. Never call from an ISR.
    virtual DiagnosticRecordResult record(DiagnosticSource source,const DiagnosticEvent& event)=0;
};
// Composition installs a sink once; absent storage defaults to a no-op recorder.
IDiagnosticRecorder& diagnosticRecorder();
void installDiagnosticRecorder(IDiagnosticRecorder* recorder);
const char* diagnosticSourceKindName(DiagnosticSourceKind kind);
}
