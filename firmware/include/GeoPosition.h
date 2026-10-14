// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#pragma once
#include <Arduino.h>
namespace EnvNode {
struct OptionalCoordinate {
    bool configured = false;
    float value = 0;
};
struct GeoPosition {
    OptionalCoordinate latitude;
    OptionalCoordinate longitude;
    OptionalCoordinate altitude;
};
bool validGeoPosition(const GeoPosition& position);
bool parseGeoPosition(const String& latitude, const String& longitude,
    const String& altitude, GeoPosition& position);
String encodeGeoPosition(const GeoPosition& position);
bool decodeGeoPosition(const String& record, GeoPosition& position);
} // namespace EnvNode
