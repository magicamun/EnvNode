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
String buildValuesHtml(const ValueRuntime& runtime, Locale locale) {
    String html;
    html.reserve(4000);
    html += (String("<section class='card'><h2>") + webText(locale, TextId::Values) + "</h2><p>" + webText(locale, TextId::SetNamedValuesForLocalUseChangesTake) + "</p>");
    html += (String("<p class='help'>") + webText(locale, TextId::ValuesDoNotControlActuatorsYet) + "</p>");
    if (runtime.values().size() < MaxEnumValueCount)
        html += (String("<div class='actions'><a class='button' href='/values/edit'>") + webText(locale, TextId::AddValue) + "</a><a class='button' href='/values/edit?example=water'>" + webText(locale, TextId::WaterSourceExample) + "</a></div>");
    html += "</section>";
    if (runtime.values().empty()) html += (String("<section class='card'><p>") + webText(locale, TextId::NoValuesConfigured) + "</p></section>");
    for (const auto& value : runtime.values()) {
        const auto& d = value.configuration();
        const auto* current = value.current();
        html += "<section class='card'><h2>"; html += escapeHtml(d.name).c_str(); html += (String("</h2><p>") + webText(locale, TextId::Current) + " " + "<strong>");
        html += escapeHtml(current ? current->label : String(webText(locale, TextId::Unknown))).c_str(); html += "</strong></p><p class='help'>";
        html += d.restartPolicy == ValueRestartPolicy::RestoreLastValue
            ? webText(locale, TextId::ChangesSurviveRestart) : webText(locale, TextId::RestartUsesTheConfiguredDefault);
        html += "</p><form method='post' action='/values/set'><input type='hidden' name='id' value='";
        html += String(d.id).c_str(); html += (String("'><label>") + webText(locale, TextId::Value) + "<select name='code'>");
        for (const auto& option : d.options) {
            html += "<option value='"; html += escapeHtml(option.code).c_str(); html += "'";
            if (current && current->code == option.code) html += " selected";
            html += ">"; html += escapeHtml(option.label).c_str(); html += "</option>";
        }
        html += (String("</select></label><div class='actions'><button>") + webText(locale, TextId::SetValue) + "</button><a class='button' href='/values/edit?id=");
        html += String(d.id).c_str(); html += (String("'>") + webText(locale, TextId::EditDefinition) + "</a></div></form></section>");
    }
    return html;
}
String buildValueEditorHtml(const EnumValueConfiguration& d, bool create, const char* error, Locale locale) {
    String html;
    html.reserve(6500);
    if (error) { html += "<p class='notice error'>"; html += escapeHtml(error).c_str(); html += "</p>"; }
    html += "<section class='card'><h2>"; html += create ? webText(locale, TextId::AddValue) : webText(locale, TextId::EditValue);
    html += "</h2><form method='post' action='/values/save'><input type='hidden' name='create' value='";
    html += create ? "1" : "0"; html += "'><input type='hidden' name='id' value='";
    html += String(d.id).c_str(); html += "'>";
    input(html, webText(locale, TextId::Name), "name", d.name, 64);
    html += (String("<label>") + webText(locale, TextId::AfterRestart) + "<select name='policy'><option value='0'");
    if (d.restartPolicy == ValueRestartPolicy::DefaultOnRestart) html += " selected";
    html += (String(">") + webText(locale, TextId::UseDefault) + "</option><option value='1'");
    if (d.restartPolicy == ValueRestartPolicy::RestoreLastValue) html += " selected";
    html += (String(">") + webText(locale, TextId::RestoreLastValue) + "</option></select></label>");
    input(html, webText(locale, TextId::DefaultOptionKey), "default", d.defaultCode, 32);
    html += (String("<p class='help'>") + webText(locale, TextId::EachOptionHasAStableKeyLowercaseLetters) + "</p>");
    html += (String("<details open><summary>") + webText(locale, TextId::OptionsUpTo16) + "</summary>");
    const size_t visibleOptions = d.options.size() < 3 ? 3 : d.options.size();
    for (unsigned int i = 0; i < 16; ++i) {
        if (i == visibleOptions) html += (String("<details><summary>") + webText(locale, TextId::MoreOptions) + "</summary>");
        const EnumValueOption empty;
        const auto& option = i < d.options.size() ? d.options[i] : empty;
        html += "<div class='grid'>";
        input(html, webText(locale, TextId::OptionKey), "code" + String(i), option.code, 32);
        input(html, webText(locale, TextId::DisplayLabel), "label" + String(i), option.label, 64);
        html += "</div>";
    }
    if (visibleOptions < 16) html += "</details>";
    html += (String("</details><p class='help'>") + webText(locale, TextId::SavingAppliesTheDefinitionImmediatelyAndSelectsIts) + "</p><div class='actions'><button>" + webText(locale, TextId::SaveDefinition) + "</button><a class='button' href='/values'>" + webText(locale, TextId::Cancel) + "</a></div></form></section>");
    if (!create) {
        html += (String("<section class='card'><details><summary>") + webText(locale, TextId::DeleteThisValue) + "</summary><p>" + webText(locale, TextId::ThisRemovesItsDefinitionAndSavedState) + "</p><form method='post' action='/values/delete'><input type='hidden' name='id' value='");
        html += String(d.id).c_str(); html += (String("'><button class='danger'>") + webText(locale, TextId::DeleteValue) + "</button></form></details></section>");
    }
    return html;
}
} // namespace EnvNode
