#include "SelectorWebView.h"
#include "WebLocalization.h"
#include "HtmlEscaping.h"
#include "JsonWriter.h"
namespace EnvNode {
String buildSelectorFields(const Configuration& configuration, const ControllerSlotConfiguration& slot,
    const String& targetOptions) {
    const Locale locale = configuration.locale.locale;
    const auto& selector = slot.implementationConfiguration.selector;
    String html((String("<div id='selectorConfiguration'><p class='help'>") + webText(locale, TextId::ChooseTheModeValueAndMapItsOptions) + "</p><label>" + webText(locale, TextId::ModeValue) + "<select id='selectorMode' name='selectorMode'><option value='0'>" + webText(locale, TextId::SelectAValue) + "</option>"));
    for (const auto& value : configuration.values) {
        String options("[");
        bool first = true;
        for (const auto& option : value.definition.options) {
            if (!first) options += ',';
            first = false;
            options += '['; appendJsonString(options, option.code.c_str()); options += ',';
            appendJsonString(options, option.label.c_str()); options += ']';
        }
        options += ']';
        html += "<option value='"; html += String(value.definition.id).c_str(); html += "' data-options='";
        html += escapeHtml(options).c_str(); html += "'";
        if (value.definition.id == selector.modeValueId) html += " selected";
        html += ">"; html += escapeHtml(value.definition.name).c_str(); html += "</option>";
    }
    html += (String("</select></label><label>") + webText(locale, TextId::AutomaticDecision) + "<select name='selectorSource'><option value='0'>" + webText(locale, TextId::SelectADecisionOnlyThreshold) + "</option>");
    for (const auto& source : configuration.controllerSlots) {
        if (source.slotId == slot.slotId || !source.enabled || source.implementation != ControllerImplementation::Threshold
            || !source.implementationConfiguration.threshold.decisionOnly) continue;
        html += "<option value='"; html += String(source.slotId).c_str(); html += "'";
        if (source.slotId == selector.automaticControllerId) html += " selected";
        html += ">"; html += escapeHtml(source.name).c_str(); html += "</option>";
    }
    html += "</select></label>";
    const char* names[] = {"selectorAuto", "selectorOn", "selectorOff"};
    const char* labels[] = {webText(locale, TextId::UseAutomaticDecisionWhen), webText(locale, TextId::ForceOutputOnWhen), webText(locale, TextId::ForceOutputOffWhen)};
    const String codes[] = {selector.automaticCode, selector.onCode, selector.offCode};
    for (size_t i = 0; i < 3; ++i) {
        html += "<label>"; html += labels[i]; html += "<select class='selectorChoice' name='"; html += names[i];
        html += "' data-current='"; html += escapeHtml(codes[i]).c_str(); html += (String("'><option value=''>") + webText(locale, TextId::SelectAnOption) + "</option></select></label>");
    }
    html += (String("<label>") + webText(locale, TextId::TargetOnOffActuator) + "<select name='selectorTarget'>"); html += targetOptions.c_str();
    html += (String("</select></label><p class='help'>") + webText(locale, TextId::MapZisterneAndHauswasserToOnOffAccording) + "</p></div>");
    html += R"JS(<script>function selectorOptions(initial){const mode=document.getElementById('selectorMode'),option=mode.options[mode.selectedIndex],choices=JSON.parse(option.dataset.options||'[]');for(const field of document.querySelectorAll('.selectorChoice')){const previous=initial?field.dataset.current:field.value;field.replaceChildren(new Option((globalThis.EnvNodeUi.SelectAnOption),''));for(const [code,label] of choices)field.add(new Option(label,code));field.value=choices.some(x=>x[0]===previous)?previous:'';}}document.getElementById('selectorMode').addEventListener('change',()=>selectorOptions(false));selectorOptions(true);</script>)JS";
    return html;
}
}
