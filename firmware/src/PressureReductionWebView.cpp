// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include "PressureReductionWebView.h"
#include "HtmlEscaping.h"
#include "WebLocalization.h"
#include <cstdio>
namespace EnvNode {
String buildPressureReductionConfigurationHtml(const Configuration& c, Locale locale) {
    String html = "<section class='card'><h2>" + String(webText(locale, TextId::PressureReduction))
        + "</h2><p>" + webText(locale, TextId::PressureTemperatureHelp)
        + "</p><form method='post' action='/pressure/save'><label>"
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
    char age[32]; snprintf(age, sizeof(age), "%u.%03u",
        static_cast<unsigned>(c.pressureReduction.maxTemperatureAgeMs / 1000),
        static_cast<unsigned>(c.pressureReduction.maxTemperatureAgeMs % 1000));
    html += "</select></label><label>" + String(webText(locale, TextId::PressureTemperatureAge))
        + "<input type='number' name='temperatureMaxAge' min='0.001' max='2147483.647' step='0.001' required value='"
        + age + "'></label><div class='actions'><button>" + webText(locale, TextId::SaveDeviceSettings)
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
    if (status.hasTemperatureAge) {
        char age[32]; snprintf(age, sizeof(age), "%u.%03u s",
            static_cast<unsigned>(status.temperatureAgeMs / 1000), static_cast<unsigned>(status.temperatureAgeMs % 1000));
        html += " · " + String(age);
    }
    html += "</span>"; return html;
}
}
