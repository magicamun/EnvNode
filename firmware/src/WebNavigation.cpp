#include "WebNavigation.h"

#include <cstring>

namespace EnvNode {

String buildWebNavigationHtml(const char* activeRoute) {
    const char* routes[][2] = {
        {"/status", "Status"}, {"/sensors", "Sensors"},
        {"/measurements", "Measurements"}, {"/display", "Display"},
        {"/actuators", "Actuators"},
        {"/controllers", "Controllers"}, {"/network", "Network"},
        {"/mqtt", "MQTT"}, {"/time", "Locale & Time"}, {"/units", "Units"},
        {"/device", "Device"}, {"/diagnostics", "Diagnostics"},
        {"/logs", "Logs"}, {"/firmware", "Firmware"},
    };
    String html;
    html.reserve(620);
    html = "<nav class='nav'>";
    for (const auto& route : routes) {
        html += "<a href='";
        html += route[0];
        html += "' class='";
        if (strcmp(activeRoute, route[0]) == 0) html += "active";
        html += "'>";
        html += route[1];
        html += "</a>";
    }
    html += "</nav>";
    return html;
}

} // namespace EnvNode
