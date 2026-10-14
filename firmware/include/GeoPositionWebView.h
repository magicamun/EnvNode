// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#pragma once
#include "GeoPosition.h"
#include "EnvNode/Locale.h"
namespace EnvNode {
String buildGeoPositionConfigurationHtml(const GeoPosition& position, Locale locale);
bool parseGeoPositionInput(const String& latitude, const String& longitude,
    const String& altitude, Locale locale, GeoPosition& position);
}
