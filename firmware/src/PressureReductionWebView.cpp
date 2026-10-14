// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include "PressureReductionWebView.h"
#include "HtmlEscaping.h"
#include "LocalizedDecimal.h"
#include "WebLocalization.h"
#include <cstdio>
namespace EnvNode {
bool parseTemperatureAgeInput(const String& input, Locale locale, uint32_t& milliseconds) {
    String canonical;
    return normalizeDecimalInput(input, locale, canonical) && parseTemperatureAgeSeconds(canonical, milliseconds);
}

String buildPressureReductionConfigurationHtml(const Configuration& c, Locale locale) {
    String html = "<section class='card'><h2>" + String(webText(locale, TextId::PressureReduction))
        + "</h2><p>" + webText(locale, TextId::PressureTemperatureHelp)
        + "</p><p>" + webText(locale, TextId::DecimalInputHelp) + "</p><form method='post' action='/pressure/save'><label>"
        + webText(locale, TextId::PressureTemperatureSource) + "<select name='temperatureSource'><option value='0'";
    if (c.pressureReduction.temperatureSensorId == InvalidSensorId) html += " selected";
    html += ">" + String(webText(locale, TextId::PressureNoTemperature)) + "</option>";
    for (const auto& slot : c.sensorSlots) {
        if (!eligiblePressureTemperatureSource(slot)) continue;
        html += "<option value='" + String(static_cast<unsigned int>(slot.slotId)) + "'";
        if (c.pressureReduction.temperatureSensorId == slot.slotId) html += " selected";
        html += ">" + String(webText(locale, TextId::Slot)) + " "
            + String(static_cast<unsigned int>(slot.slotId)) + " · " + escapeHtml(slot.name);
        const auto* metadata = SensorImplementationRegistry::find(slot.implementation);
        if (metadata->provenance == SensorProvenance::Simulated)
            html += " (" + String(webText(locale, TextId::Simulated)) + ")";
        html += "</option>";
    }
    html += "</select></label><label>" + String(webText(locale, TextId::PressureTemperatureAge))
        + "<input type='text' inputmode='decimal' maxlength='32' name='temperatureMaxAge' lang='"
        + localeKey(locale) + "' required value='"
        + formatDecimalInput(c.pressureReduction.maxTemperatureAgeMs / 1000.0, 3, locale)
        + "'></label><div class='actions'><button>" + webText(locale, TextId::SaveDeviceSettings)
        + "</button></div></form></section>";
    return html;
}
String buildPressureReductionDiagnosticHtml(const PressureReductionStatus& status, Locale locale) {
    TextId reason = TextId::PressureUsingTemperature;
    switch (status.reason) {
        case PressureReductionReason::NoAltitude: reason = TextId::PressureNoAltitude; break;
        case PressureReductionReason::InvalidPressure: reason = TextId::PressureInvalidInput; break;
        case PressureReductionReason::TemperatureNotSelected: reason = TextId::PressureNoTemperature; break;
        case PressureReductionReason::TemperatureUnavailable: reason = TextId::PressureTemperatureUnavailable; break;
        case PressureReductionReason::TemperatureInvalid: reason = TextId::PressureTemperatureInvalid; break;
        case PressureReductionReason::TemperatureStale: reason = TextId::PressureTemperatureStale; break;
        case PressureReductionReason::TemperatureQuality: reason = TextId::PressureTemperatureQuality; break;
        default: break;
    }
    String html = "<span class='secondary'>" + String(webText(locale,
        status.method == PressureReductionMethod::LocalTemperature ? TextId::PressureMethodTemperature
            : status.method == PressureReductionMethod::StandardAtmosphere ? TextId::PressureMethodStandard
            : TextId::Invalid)) + " · " + webText(locale, reason);
    if (status.temperatureSensorId != InvalidSensorId)
        html += " · " + String(webText(locale, TextId::Slot)) + " "
            + String(static_cast<unsigned int>(status.temperatureSensorId));
    if (status.hasTemperatureAge)
        html += " · " + formatDecimalInput(status.temperatureAgeMs / 1000.0, 3, locale) + " s";
    html += "</span>"; return html;
}
}
