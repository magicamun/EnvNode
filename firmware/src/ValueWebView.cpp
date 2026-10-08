#include "ValueWebView.h"
#include "HtmlEscaping.h"

namespace EnvNode {
bool parseValueId(const String& text, ValueId& id) {
    if (text.length() == 0 || text.length() > 5) return false;
    unsigned int number = 0;
    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
        number = number * 10 + text[i] - '0';
        if (number > 65535) return false;
    }
    if (number == 0) return false;
    id = static_cast<ValueId>(number);
    return true;
}
namespace {
void input(String& html, const char* label, const String& name, const String& value, unsigned int length) {
    html += "<label>"; html += label;
    html += "<input name='"; html += name.c_str();
    html += "' maxlength='"; html += String(length).c_str();
    html += "' value='"; html += escapeHtml(value).c_str(); html += "'></label>";
}
}
String buildValuesHtml(const ValueRuntime& runtime) {
    String html;
    html.reserve(4000);
    html += "<section class='card'><h2>Values</h2><p>Set named values for local use. Changes take effect immediately.</p>";
    html += "<p class='help'>Values do not control actuators yet.</p>";
    if (runtime.values().size() < MaxEnumValueCount)
        html += "<div class='actions'><a class='button' href='/values/edit'>Add value</a><a class='button' href='/values/edit?example=water'>Water source example</a></div>";
    html += "</section>";
    if (runtime.values().empty()) html += "<section class='card'><p>No Values configured.</p></section>";
    for (const auto& value : runtime.values()) {
        const auto& d = value.configuration();
        const auto* current = value.current();
        html += "<section class='card'><h2>"; html += escapeHtml(d.name).c_str(); html += "</h2><p>Current: <strong>";
        html += escapeHtml(current ? current->label : String("Unknown")).c_str(); html += "</strong></p><p class='help'>";
        html += d.restartPolicy == ValueRestartPolicy::RestoreLastValue
            ? "Changes survive restart." : "Restart uses the configured default.";
        html += "</p><form method='post' action='/values/set'><input type='hidden' name='id' value='";
        html += String(d.id).c_str(); html += "'><label>Value<select name='code'>";
        for (const auto& option : d.options) {
            html += "<option value='"; html += escapeHtml(option.code).c_str(); html += "'";
            if (current && current->code == option.code) html += " selected";
            html += ">"; html += escapeHtml(option.label).c_str(); html += "</option>";
        }
        html += "</select></label><div class='actions'><button>Set value</button><a class='button' href='/values/edit?id=";
        html += String(d.id).c_str(); html += "'>Edit definition</a></div></form></section>";
    }
    return html;
}
String buildValueEditorHtml(const EnumValueConfiguration& d, bool create, const char* error) {
    String html;
    html.reserve(6500);
    if (error) { html += "<p class='notice error'>"; html += escapeHtml(error).c_str(); html += "</p>"; }
    html += "<section class='card'><h2>"; html += create ? "Add value" : "Edit value";
    html += "</h2><form method='post' action='/values/save'><input type='hidden' name='create' value='";
    html += create ? "1" : "0"; html += "'><input type='hidden' name='id' value='";
    html += String(d.id).c_str(); html += "'>";
    input(html, "Name", "name", d.name, 64);
    html += "<label>After restart<select name='policy'><option value='0'";
    if (d.restartPolicy == ValueRestartPolicy::DefaultOnRestart) html += " selected";
    html += ">Use default</option><option value='1'";
    if (d.restartPolicy == ValueRestartPolicy::RestoreLastValue) html += " selected";
    html += ">Restore last value</option></select></label>";
    input(html, "Default option key", "default", d.defaultCode, 32);
    html += "<p class='help'>Each option has a stable key (lowercase letters, digits, underscore) and a display label. Enter one of these keys as the default. Leave unused rows empty.</p>";
    html += "<details open><summary>Options (up to 16)</summary>";
    const size_t visibleOptions = d.options.size() < 3 ? 3 : d.options.size();
    for (unsigned int i = 0; i < 16; ++i) {
        if (i == visibleOptions) html += "<details><summary>More options</summary>";
        const EnumValueOption empty;
        const auto& option = i < d.options.size() ? d.options[i] : empty;
        html += "<div class='grid'>";
        input(html, "Option key", "code" + String(i), option.code, 32);
        input(html, "Display label", "label" + String(i), option.label, 64);
        html += "</div>";
    }
    if (visibleOptions < 16) html += "</details>";
    html += "</details><p class='help'>Saving applies the definition immediately and selects its saved value or default. Other Values keep their current state. Names and labels allow up to 64 UTF-8 bytes.</p><div class='actions'><button>Save definition</button><a class='button' href='/values'>Cancel</a></div></form></section>";
    if (!create) {
        html += "<section class='card'><details><summary>Delete this value</summary><p>This removes its definition and saved state.</p><form method='post' action='/values/delete'><input type='hidden' name='id' value='";
        html += String(d.id).c_str(); html += "'><button class='danger'>Delete value</button></form></details></section>";
    }
    return html;
}
} // namespace EnvNode
