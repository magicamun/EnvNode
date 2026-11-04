// EnvNode: see repository LICENSE.
#pragma once
#include "TestMarkerSession.h"
#include "WebLocalization.h"
#include <Arduino.h>
namespace EnvNode {
String buildTestMarkerHtml(const TestMarkerSession&,Locale,bool available,const char* bootId);
bool matchingTestMarkerBoot(const String&,const char* currentBoot);
bool parseTestMarkerId(const String&,uint32_t&);
int testMarkerHttpStatus(TestMarkerResult);
const char* testMarkerResultText(TestMarkerResult,Locale);
}
