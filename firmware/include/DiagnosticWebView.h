// EnvNode: see repository LICENSE.
#pragma once
#include <Arduino.h>
#include "EnvNode/Locale.h"
namespace EnvNode {
bool extendedDiagnosticsChoice(bool previous,bool cardAvailable,bool renderedEditable,bool submitted);
String renderExtendedDiagnosticsCheckbox(bool enabled,bool cardAvailable,Locale locale);
}
