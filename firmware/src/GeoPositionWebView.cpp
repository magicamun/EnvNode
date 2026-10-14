// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include "GeoPositionWebView.h"
#include "LocalizedDecimal.h"
#include "WebLocalization.h"
namespace EnvNode {
String buildGeoPositionConfigurationHtml(const GeoPosition& position, Locale locale) {
    String html = "<section class='card'><h2>" + String(webText(locale, TextId::GeoPosition))
        + "</h2><p>" + webText(locale, TextId::GeoPositionHelp) + "</p><p>"
        + webText(locale, TextId::DecimalInputHelp)
        + "</p><form method='post' action='/location/save'>";
    const auto input = [&html, locale](TextId label, const char* name,
        const OptionalCoordinate& coordinate, unsigned precision) {
        html += "<label>" + String(webText(locale, label)) + "<input type='text' inputmode='decimal' maxlength='32' lang='"
            + localeKey(locale) + "' name='" + name + "' value='";
        if (coordinate.configured) html += formatDecimalInput(coordinate.value, precision, locale);
        html += "'></label>";
    };
    input(TextId::Latitude, "latitude", position.latitude, 7);
    input(TextId::Longitude, "longitude", position.longitude, 7);
    input(TextId::Altitude, "altitude", position.altitude, 3);
    html += "<div class='actions'><button>" + String(webText(locale, TextId::SaveDeviceSettings))
        + "</button></div></form></section>";
    return html;
}
bool parseGeoPositionInput(const String& lat, const String& lon, const String& alt,
    Locale locale, GeoPosition& position) {
    String canonicalLatitude, canonicalLongitude, canonicalAltitude;
    return normalizeDecimalInput(lat, locale, canonicalLatitude)
        && normalizeDecimalInput(lon, locale, canonicalLongitude)
        && normalizeDecimalInput(alt, locale, canonicalAltitude)
        && parseGeoPosition(canonicalLatitude, canonicalLongitude, canonicalAltitude, position);
}
}
