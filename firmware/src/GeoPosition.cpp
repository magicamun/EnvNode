// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include "GeoPosition.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
namespace EnvNode {
namespace {
bool valid(const OptionalCoordinate& value, float minimum, float maximum) {
    return std::isfinite(value.value) && (!value.configured
        || (value.value >= minimum && value.value <= maximum));
}
bool parse(const String& input, OptionalCoordinate& value) {
    value = OptionalCoordinate{};
    if (input.isEmpty()) return true;
    // Accept decimal degrees/metres only, without implicit whitespace or hex coercion.
    bool digit = false, dot = false;
    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];
        if (i == 0 && (c == '-' || c == '+')) continue;
        if (c == '.' && !dot) { dot = true; continue; }
        if (c < '0' || c > '9') return false;
        digit = true;
    }
    if (!digit || input.length() > 32) return false;
    char* end = nullptr;
    value.value = strtof(input.c_str(), &end);
    value.configured = end != input.c_str() && *end == 0 && std::isfinite(value.value);
    return value.configured;
}
String encode(const OptionalCoordinate& value) {
    if (!value.configured) return String();
    char buffer[48]; snprintf(buffer, sizeof(buffer), "%.9f", static_cast<double>(value.value));
    return String(buffer);
}
}
bool validGeoPosition(const GeoPosition& p) {
    return valid(p.latitude, -90, 90) && valid(p.longitude, -180, 180)
        && valid(p.altitude, -1000, 10000);
}
bool parseGeoPosition(const String& lat, const String& lon, const String& alt, GeoPosition& position) {
    GeoPosition candidate;
    if (!parse(lat, candidate.latitude) || !parse(lon, candidate.longitude)
        || !parse(alt, candidate.altitude) || !validGeoPosition(candidate)) return false;
    position = candidate; return true;
}
String encodeGeoPosition(const GeoPosition& p) {
    return encode(p.latitude) + ";" + encode(p.longitude) + ";" + encode(p.altitude);
}
bool decodeGeoPosition(const String& record, GeoPosition& position) {
    const char* start = record.c_str();
    const char* first = strchr(start, ';');
    const char* second = first ? strchr(first + 1, ';') : nullptr;
    if (!second || strchr(second + 1, ';')) return false;
    String lat, lon;
    for (const char* c = start; c < first; ++c) lat += *c;
    for (const char* c = first + 1; c < second; ++c) lon += *c;
    return parseGeoPosition(lat, lon, String(second + 1), position);
}
}
