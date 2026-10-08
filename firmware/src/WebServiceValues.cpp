#include "WebService.h"
#include "ValueWebView.h"

namespace EnvNode {
void WebService::handleValues() {
    server_.sendHeader("Cache-Control", "no-store");
    sendPage("Values", "/values", buildValuesHtml(valueRuntime_));
}
void WebService::handleValueEdit() {
    server_.sendHeader("Cache-Control", "no-store");
    EnumValueConfiguration definition;
    const bool create = !server_.hasArg("id");
    if (!create) {
        ValueId id;
        if (!parseValueId(server_.arg("id"), id) || !valueRuntime_.find(id)) {
            sendResult("Unknown value", "/values", "This Value no longer exists.", false); return;
        }
        definition = valueRuntime_.find(id)->configuration();
    } else {
        if (valueRuntime_.values().size() >= MaxEnumValueCount) {
            sendResult("Values full", "/values", "At most eight Values can be configured.", false); return;
        }
        definition.id = 1;
        while (valueRuntime_.find(definition.id)) ++definition.id;
        if (server_.arg("example") == "water") {
            definition.name = "Wasserquelle";
            definition.options = {{"auto", "Automatik"}, {"cistern", "Zisterne"}, {"mains", "Hauswasser"}};
            definition.defaultCode = "auto";
        }
    }
    sendPage("Value definition", "/values", buildValueEditorHtml(definition, create));
}
void WebService::handleValueSave() {
    EnumValueConfiguration definition;
    if (!parseValueId(server_.arg("id"), definition.id)
        || (server_.arg("create") != "0" && server_.arg("create") != "1")
        || (server_.arg("policy") != "0" && server_.arg("policy") != "1")) {
        sendResult("Invalid Value", "/values", "Invalid or incomplete form.", false); return;
    }
    if (!server_.hasArg("name") || !server_.hasArg("default")) {
        sendResult("Invalid Value", "/values", "Incomplete definition. Previous state retained.", false); return;
    }
    for (unsigned int i = 0; i < 16; ++i) {
        if (!server_.hasArg("code" + String(i)) || !server_.hasArg("label" + String(i))) {
            sendResult("Invalid Value", "/values", "Incomplete options. Previous state retained.", false); return;
        }
    }
    const bool create = server_.arg("create") == "1";
    definition.name = server_.arg("name");
    definition.defaultCode = server_.arg("default");
    definition.restartPolicy = server_.arg("policy") == "1"
        ? ValueRestartPolicy::RestoreLastValue : ValueRestartPolicy::DefaultOnRestart;
    for (unsigned int i = 0; i < 16; ++i) {
        EnumValueOption option;
        option.code = server_.arg("code" + String(i));
        option.label = server_.arg("label" + String(i));
        if (!option.code.isEmpty() || !option.label.isEmpty()) definition.options.push_back(option);
    }
    const char* error = nullptr;
    if (!validEnumValueConfiguration(definition))
        error = "Use a name, unique option keys with labels, and an existing key as the default. Keys allow 32 bytes; names and labels 64 UTF-8 bytes. Control characters are not allowed.";
    else if (!valueRuntime_.saveDefinition(definition, create))
        error = "Could not save. An enabled Selector may require these options, the Value may have changed, capacity may be full, or storage failed. Previous state retained.";
    if (error) { sendPage("Value save failed", "/values", buildValueEditorHtml(definition, create, error), 400); return; }
    server_.sendHeader("Location", "/values"); server_.send(303, "text/plain", "");
}
void WebService::handleValueSet() {
    ValueId id;
    if (!parseValueId(server_.arg("id"), id)) {
        sendResult("Invalid Value", "/values", "Invalid Value ID.", false); return;
    }
    const auto result = valueRuntime_.set(id, server_.arg("code"));
    if (result != ValueCommandResult::Changed && result != ValueCommandResult::Unchanged) {
        sendResult("Value unchanged", "/values", result == ValueCommandResult::StorageFailed
            ? "Could not store the new value. Previous state retained." : "Unknown Value or option. Reload the Values page.", false); return;
    }
    server_.sendHeader("Location", "/values"); server_.send(303, "text/plain", "");
}
void WebService::handleValueDelete() {
    ValueId id;
    if (!parseValueId(server_.arg("id"), id) || !valueRuntime_.remove(id)) {
        sendResult("Delete failed", "/values", "Unknown Value, referenced by an enabled Selector, or storage failure. Previous state retained.", false); return;
    }
    server_.sendHeader("Location", "/values"); server_.send(303, "text/plain", "");
}
} // namespace EnvNode
