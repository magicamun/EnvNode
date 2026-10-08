#include "WebService.h"
#include "ValueWebView.h"
namespace EnvNode {
bool WebService::readSelectorConfiguration(ControllerSlotConfiguration& slot) {
    auto& selector = slot.implementationConfiguration.selector;
    if (!parseValueId(server_.arg("selectorMode"), selector.modeValueId)
        || !parseValueId(server_.arg("selectorSource"), selector.automaticControllerId)) return false;
    selector.automaticCode = server_.arg("selectorAuto");
    selector.onCode = server_.arg("selectorOn");
    selector.offCode = server_.arg("selectorOff");
    const String target = server_.arg("selectorTarget");
    if (target == "m:current") return validModuleActuatorReference(slot.moduleTarget);
    slot.moduleTarget = ModuleActuatorReference{};
    if (target.startsWith("m:")) {
        uint16_t id;
        if (!parseValueId(target.substring(2), id)) return false;
        selector.targetActuatorId = id;
        return actuatorRuntime_.moduleReference(id, slot.moduleTarget);
    }
    return parseValueId(target, selector.targetActuatorId);
}
}
