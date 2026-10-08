#include "SelectorWebView.h"
#include "HtmlEscaping.h"
#include "JsonWriter.h"
namespace EnvNode {
String buildSelectorFields(const Configuration& configuration, const ControllerSlotConfiguration& slot,
    const String& targetOptions) {
    const auto& selector = slot.implementationConfiguration.selector;
    String html("<div id='selectorConfiguration'><p class='help'>Choose the mode Value and map its options. The Selector alone controls the target. Unknown input holds its current state.</p><label>Mode Value<select id='selectorMode' name='selectorMode'><option value='0'>Select a Value</option>");
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
    html += "</select></label><label>Automatic decision<select name='selectorSource'><option value='0'>Select a decision-only Threshold</option>";
    for (const auto& source : configuration.controllerSlots) {
        if (source.slotId == slot.slotId || !source.enabled || source.implementation != ControllerImplementation::Threshold
            || !source.implementationConfiguration.threshold.decisionOnly) continue;
        html += "<option value='"; html += String(source.slotId).c_str(); html += "'";
        if (source.slotId == selector.automaticControllerId) html += " selected";
        html += ">"; html += escapeHtml(source.name).c_str(); html += "</option>";
    }
    html += "</select></label>";
    const char* names[] = {"selectorAuto", "selectorOn", "selectorOff"};
    const char* labels[] = {"Use automatic decision when", "Force output On when", "Force output Off when"};
    const String codes[] = {selector.automaticCode, selector.onCode, selector.offCode};
    for (size_t i = 0; i < 3; ++i) {
        html += "<label>"; html += labels[i]; html += "<select class='selectorChoice' name='"; html += names[i];
        html += "' data-current='"; html += escapeHtml(codes[i]).c_str(); html += "'><option value=''>Select an option</option></select></label>";
    }
    html += "<label>Target On/Off actuator<select name='selectorTarget'>"; html += targetOptions.c_str();
    html += "</select></label><p class='help'>Map Zisterne and Hauswasser to On/Off according to your valve wiring. Missing/invalid automatic input and unmapped modes cause no output command. Stop also holds the output.</p></div>";
    html += R"JS(<script>function selectorOptions(initial){const mode=document.getElementById('selectorMode'),option=mode.options[mode.selectedIndex],choices=JSON.parse(option.dataset.options||'[]');for(const field of document.querySelectorAll('.selectorChoice')){const previous=initial?field.dataset.current:field.value;field.replaceChildren(new Option('Select an option',''));for(const [code,label] of choices)field.add(new Option(label,code));field.value=choices.some(x=>x[0]===previous)?previous:'';}}document.getElementById('selectorMode').addEventListener('change',()=>selectorOptions(false));selectorOptions(true);</script>)JS";
    return html;
}
}
