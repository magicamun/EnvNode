#include "ValueStatePublisher.h"
#include "ValueMqttTopic.h"
#include "JsonWriter.h"
namespace EnvNode {
String buildValueMqttDescription(const String& deviceName, const EnumValueConfiguration& d) {
    String json("{\"schema_version\":1,\"id\":"); json += String(d.id).c_str();
    json += ",\"name\":"; appendJsonString(json, d.name.c_str());
    json += ",\"value_type\":\"enum\",\"default\":"; appendJsonString(json, d.defaultCode.c_str());
    json += ",\"restart_policy\":"; appendJsonString(json, d.restartPolicy == ValueRestartPolicy::RestoreLastValue ? "restore_last" : "default");
    json += ",\"command_topic\":"; appendJsonString(json, mqttValueCommandTopic(deviceName, d.id).c_str());
    json += ",\"state_topic\":"; appendJsonString(json, mqttValueStatusTopic(deviceName, d.id).c_str());
    json += ",\"options\":[";
    bool first = true;
    for (const auto& option : d.options) {
        if (!first) json += ',';
        first = false;
        json += "{\"code\":"; appendJsonString(json, option.code.c_str());
        json += ",\"label\":"; appendJsonString(json, option.label.c_str()); json += '}';
    }
    json += "]}";
    return json;
}
void ValueStatePublisher::loop() {
    if (!mqtt_.connected()) { wasConnected_ = false; return; }
    if (!wasConnected_) {
        for (auto& item : published_) { item.stateKnown = false; item.descriptionKnown = false; }
        wasConnected_ = true;
    }
    const String& name = configuration_.getConfiguration().device.name;
    bool failed = false;
    for (auto it = published_.begin(); it != published_.end();) {
        if (it->deviceName == name && values_.find(it->id)) { ++it; continue; }
        if (!it->stateCleared) it->stateCleared = mqtt_.publish(mqttValueStatusTopic(it->deviceName, it->id).c_str(), "", true);
        if (!it->descriptionCleared) it->descriptionCleared = mqtt_.publish(mqttValueDescriptionTopic(it->deviceName, it->id).c_str(), "", true);
        if (it->stateCleared && it->descriptionCleared) it = published_.erase(it);
        else { failed = true; ++it; }
    }
    for (const auto& value : values_.values()) {
        const auto& d = value.configuration();
        Published* item = nullptr;
        for (auto& candidate : published_) if (candidate.id == d.id && candidate.deviceName == name) { item = &candidate; break; }
        if (!item) {
            // Keep retry state bounded even if removals fail during rapid edits.
            if (published_.size() >= MaxEnumValueCount) { failed = true; continue; }
            Published fresh; fresh.id = d.id; fresh.deviceName = name;
            published_.push_back(fresh); item = &published_.back();
        }
        if (item->stateCleared || item->descriptionCleared) { item->stateKnown = false; item->descriptionKnown = false; }
        item->stateCleared = false; item->descriptionCleared = false;
        const auto* current = value.current();
        if (current && (!item->stateKnown || item->state != current->code)) {
            if (mqtt_.publish(mqttValueStatusTopic(name, d.id).c_str(), current->code.c_str(), true)) {
                item->state = current->code; item->stateKnown = true;
            } else failed = true;
        }
        if (!item->descriptionKnown || item->descriptionRevision != values_.compositionRevision()) {
            const String description = buildValueMqttDescription(name, d);
            if (mqtt_.publish(mqttValueDescriptionTopic(name, d.id).c_str(), description.c_str(), true)) {
                item->descriptionRevision = values_.compositionRevision(); item->descriptionKnown = true;
            } else failed = true;
        }
    }
    if (failed && !failureReported_) logger_.warn("MQTT Value publication failed; retry pending");
    if (!failed && failureReported_) logger_.info("MQTT Value publication recovered");
    failureReported_ = failed;
}
}
