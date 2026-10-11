#include "WebNavigation.h"
#include "WebLocalization.h"

#include <cstring>

namespace EnvNode {

String buildWebNavigationHtml(const char* activeRoute, Locale locale) {
    struct Route { const char* path; TextId text; };
    const Route routes[] = {
        {"/status", TextId::Status}, {"/sensors", TextId::Sensors},
        {"/measurements", TextId::Measurements}, {"/display", TextId::Display},
        {"/values", TextId::Values}, {"/actuators", TextId::Actuators},
        {"/controllers", TextId::Controllers}, {"/network", TextId::Network},
        {"/mqtt", TextId::MQTT}, {"/time", TextId::LocaleTime}, {"/units", TextId::Units},
        {"/device", TextId::Device}, {"/diagnostics", TextId::Diagnostics},
        {"/logs", TextId::Logs}, {"/firmware", TextId::Firmware},
    };
    String html;
    html.reserve(620);
    html = "<nav class='nav'>";
    for (const auto& route : routes) {
        html += "<a href='";
        html += route.path;
        html += "' class='";
        if (strcmp(activeRoute, route.path) == 0) html += "active";
        html += "'>";
        html += webText(locale, route.text);
        html += "</a>";
    }
    html += "</nav>";
    return html;
}

} // namespace EnvNode
