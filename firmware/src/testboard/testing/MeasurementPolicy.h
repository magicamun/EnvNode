// EnvNode: see repository LICENSE.
#pragma once
#include "TestModel.h"
namespace EnvNode { namespace Testing {
struct DirectedRule { const char* from; const char* to; };
struct MeasurementPolicy { const char* id; const DirectedRule* rules; size_t count; };
// Reviewed measurement interpretation, separate from immutable schema-1 profile bytes.
// Match the full electrical interface to the canonical built-in definition.
const MeasurementPolicy* measurementPolicy(const BoardDefinition& board);
int forwardRule(const MeasurementPolicy* policy,const char* source,const char* target);
int reverseRule(const MeasurementPolicy* policy,const char* source,const char* target);
} }
