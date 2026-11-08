// EnvNode: see repository LICENSE.
#pragma once
#include "TestModel.h"
namespace EnvNode { namespace Testing {
constexpr size_t MaxDirectedRules = 3;
// The forward measurement may end behind a protection diode. Reverse excitation
// uses the actual rail on a pigtail contact, never the protected reference input.
struct DirectedRule { const char* from; const char* to; const char* reverseSource; };
struct MeasurementPolicy { const char* id; const DirectedRule* rules; size_t count; };
// Reviewed measurement interpretation, separate from immutable schema-1 profile bytes.
// Match the full electrical interface to the canonical built-in definition.
const MeasurementPolicy* measurementPolicy(const BoardDefinition& board);
int forwardRule(const MeasurementPolicy* policy,const char* source,const char* target);
int reverseRule(const MeasurementPolicy* policy,const char* source,const char* target);
} }
