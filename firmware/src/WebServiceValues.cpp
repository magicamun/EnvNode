#include "WebService.h"
#include "ValueWebView.h"

namespace EnvNode {
void WebService::handleValues() {
    server_.sendHeader("Cache-Control", "no-store");
    sendPage(uiText(TextId::Values), "/values", buildValuesHtml(valueRuntime_, configurationService_.getLocale()));
}
void WebService::handleValueEdit() {
    server_.sendHeader("Cache-Control", "no-store");
    EnumValueConfiguration definition;
    const bool create = !server_.hasArg("id");
    if (!create) {
        ValueId id;
        if (!parseValueId(server_.arg("id"), id) || !valueRuntime_.find(id)) {
            sendResult(uiText(TextId::UnknownValue), "/values", uiText(TextId::ThisValueNoLongerExists), false); return;
        }
        definition = valueRuntime_.find(id)->configuration();
    } else {
        if (valueRuntime_.values().size() >= MaxEnumValueCount) {
            sendResult(uiText(TextId::ValuesFull), "/values", uiText(TextId::AtMostEightValuesCanBeConfigured), false); return;
        }
        definition.id = 1;
        while (valueRuntime_.find(definition.id)) ++definition.id;
        if (server_.arg("example") == "water") {
            definition.name = "Wasserquelle";
            definition.options = {{"auto", "Automatik"}, {"cistern", "Zisterne"}, {"mains", "Hauswasser"}};
            definition.defaultCode = "auto";
        }
    }
    sendPage(uiText(TextId::ValueDefinition), "/values", buildValueEditorHtml(definition, create, nullptr, configurationService_.getLocale()));
}
void WebService::handleValueSave() {
    EnumValueConfiguration definition;
    if (!parseValueId(server_.arg("id"), definition.id)
        || (server_.arg("create") != "0" && server_.arg("create") != "1")
        || (server_.arg("policy") != "0" && server_.arg("policy") != "1")) {
        sendResult(uiText(TextId::InvalidValue), "/values", uiText(TextId::InvalidOrIncompleteForm), false); return;
    }
    if (!server_.hasArg("name") || !server_.hasArg("default")) {
        sendResult(uiText(TextId::InvalidValue), "/values", uiText(TextId::IncompleteDefinitionPreviousStateRetained), false); return;
    }
    for (unsigned int i = 0; i < 16; ++i) {
        if (!server_.hasArg("code" + String(i)) || !server_.hasArg("label" + String(i))) {
            sendResult(uiText(TextId::InvalidValue), "/values", uiText(TextId::IncompleteOptionsPreviousStateRetained), false); return;
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
        error = uiText(TextId::UseANameUniqueOptionKeysWithLabels);
    else if (!valueRuntime_.saveDefinition(definition, create))
        error = uiText(TextId::CouldNotSaveAnEnabledSelectorMayRequire);
    if (error) { sendPage(uiText(TextId::ValueSaveFailed), "/values", buildValueEditorHtml(definition, create, error, configurationService_.getLocale()), 400); return; }
    server_.sendHeader(uiText(TextId::Location), "/values"); server_.send(303, "text/plain", "");
}
void WebService::handleValueSet() {
    ValueId id;
    if (!parseValueId(server_.arg("id"), id)) {
        sendResult(uiText(TextId::InvalidValue), "/values", uiText(TextId::InvalidValueID), false); return;
    }
    const auto result = valueRuntime_.set(id, server_.arg("code"));
    if (result != ValueCommandResult::Changed && result != ValueCommandResult::Unchanged) {
        sendResult(uiText(TextId::ValueUnchanged), "/values", result == ValueCommandResult::StorageFailed
            ? uiText(TextId::CouldNotStoreTheNewValuePreviousState) : uiText(TextId::UnknownValueOrOptionReloadTheValuesPage), false); return;
    }
    server_.sendHeader(uiText(TextId::Location), "/values"); server_.send(303, "text/plain", "");
}
void WebService::handleValueDelete() {
    ValueId id;
    if (!parseValueId(server_.arg("id"), id) || !valueRuntime_.remove(id)) {
        sendResult(uiText(TextId::DeleteFailed), "/values", uiText(TextId::UnknownValueReferencedByAnEnabledSelectorOr), false); return;
    }
    server_.sendHeader(uiText(TextId::Location), "/values"); server_.send(303, "text/plain", "");
}
} // namespace EnvNode
