// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#pragma once
#include <Arduino.h>
#include "EnvNode/Locale.h"
namespace EnvNode {
// Editable decimals have no grouping. Protocol/storage codecs remain canonical.
String formatDecimalInput(double value, unsigned precision, Locale locale);
bool normalizeDecimalInput(const String& input, Locale locale, String& canonical);
}
