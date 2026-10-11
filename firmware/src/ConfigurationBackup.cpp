// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include "ConfigurationBackup.h"
#include "BoardProfile.h"
#include "JsonWriter.h"
#include <vector>
#include <memory>
#include <string>
#include <limits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace EnvNode {
namespace {
// Bounded, string-only JSON object. No recursive parsing or implicit numeric coercion.
class JsonObject {
public:
    explicit JsonObject(const String& input) : input_(input) {}
    bool parse() {
        if (!take('{')) return false;
        do {
            String key, value;
            const size_t keyPosition = position_;
            if (!quoted(key, 100) || !take(':')) return false;
            const size_t valuePosition = position_;
            if (!quoted(value, 30000)) return false;
            fields_.push_back({hash(key), keyPosition, valuePosition, false});
            if (fields_.size() > 1600) return false;
            space();
            if (position_ < input_.length() && input_[position_] == '}') {
                ++position_; space(); return position_ == input_.length();
            }
        } while (take(','));
        return false;
    }
    bool get(const String& key, String& value) {
        const uint32_t wanted = hash(key);
        for (auto& field : fields_) {
            if (field.used || field.hash != wanted) continue;
            position_ = field.keyPosition;
            String actual;
            if (!quoted(actual, 100) || actual != key) continue;
            position_ = field.valuePosition;
            value = String();
            if (!quoted(value, 30000)) return false;
            field.used = true; return true;
        }
        return false;
    }
    bool empty() const { for (const auto& field : fields_) if (!field.used) return false; return true; }
private:
    static bool validUtf8(const String& text) {
        for (size_t i = 0; i < text.length();) {
            uint32_t code = static_cast<uint8_t>(text[i++]);
            if (code < 0x80) continue;
            unsigned count; uint32_t minimum;
            if (code >= 0xc2 && code <= 0xdf) { count = 1; minimum = 0x80; code &= 0x1f; }
            else if (code >= 0xe0 && code <= 0xef) { count = 2; minimum = 0x800; code &= 0xf; }
            else if (code >= 0xf0 && code <= 0xf4) { count = 3; minimum = 0x10000; code &= 7; }
            else return false;
            if (count > text.length() - i) return false;
            while (count--) {
                uint8_t next = static_cast<uint8_t>(text[i++]);
                if ((next & 0xc0) != 0x80) return false;
                code = (code << 6) | (next & 0x3f);
            }
            if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)) return false;
        }
        return true;
    }
    void space() { while (position_ < input_.length() &&
        (input_[position_] == ' ' || input_[position_] == '\n' || input_[position_] == '\r' || input_[position_] == '\t')) ++position_; }
    bool take(char c) { space(); if (position_ >= input_.length() || input_[position_] != c) return false; ++position_; return true; }
    bool hex(unsigned& value) {
        value = 0;
        for (int i = 0; i < 4; ++i) {
            if (position_ >= input_.length()) return false;
            char c = input_[position_++];
            int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
            if (digit < 0) return false;
            value = value * 16 + digit;
        }
        return true;
    }
    bool quoted(String& value, size_t maximum) {
        if (!take('"')) return false;
        while (position_ < input_.length()) {
            unsigned char c = input_[position_++];
            if (c == '"') return validUtf8(value);
            if (c < 32) return false;
            if (c == '\\') {
                if (position_ >= input_.length()) return false;
                c = input_[position_++];
                switch (c) {
                    case '"': case '\\': case '/': value += static_cast<char>(c); break;
                    case 'n': value += '\n'; break;
                    case 'r': value += '\r'; break;
                    case 't': value += '\t'; break;
                    case 'b': value += '\b'; break;
                    case 'f': value += '\f'; break;
                    case 'u': {
                        unsigned code;
                        if (!hex(code) || code == 0) return false;
                        if (code >= 0xd800 && code <= 0xdbff) {
                            unsigned low;
                            if (position_ + 2 > input_.length() || input_[position_++] != '\\'
                                || input_[position_++] != 'u' || !hex(low) || low < 0xdc00 || low > 0xdfff) return false;
                            code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
                        } else if (code >= 0xdc00 && code <= 0xdfff) return false;
                        if (code < 0x80) value += static_cast<char>(code);
                        else {
                            if (code >= 0x10000) value += static_cast<char>(0xf0 | (code >> 18));
                            if (code >= 0x800) value += static_cast<char>((code >= 0x10000 ? 0x80 : 0xe0) | ((code >> 12) & (code >= 0x10000 ? 0x3f : 0xf)));
                            value += static_cast<char>((code >= 0x800 ? 0x80 : 0xc0) | ((code >> 6) & (code >= 0x800 ? 0x3f : 0x1f)));
                            value += static_cast<char>(0x80 | (code & 0x3f));
                        }
                        break;
                    }
                    default: return false;
                }
            } else value += static_cast<char>(c);
            if (value.length() > maximum) return false;
        }
        return false;
    }
    static uint32_t hash(const String& text) {
        uint32_t result = 2166136261u;
        for (size_t i = 0; i < text.length(); ++i) result = (result ^ static_cast<uint8_t>(text[i])) * 16777619u;
        return result;
    }
    struct Field { uint32_t hash; size_t keyPosition; size_t valuePosition; bool used; };
    const String& input_;
    size_t position_ = 0;
    std::vector<Field> fields_;
};
class Fields {
public:
    Fields(String& output) : output_(&output) {}
    Fields(JsonObject& input) : input_(&input) {}
    Fields(std::vector<uint8_t>& output, bool) : recordOutput_(&output) {}
    Fields(const std::vector<uint8_t>& input) : recordInput_(&input) {}
    bool reading() const { return input_ != nullptr || recordInput_ != nullptr; }
    bool complete() const { return !recordInput_ || recordPosition_ == recordInput_->size(); }
    bool ok = true;
    void text(const String& key, String& value) {
        if (!ok) return;
        if (input_) { ok = input_->get(key, value); return; }
        if (recordOutput_) {
            if (value.length() > 30000 || strlen(value.c_str()) != value.length()) { ok = false; return; }
            recordOutput_->push_back(static_cast<uint8_t>(value.length()));
            recordOutput_->push_back(static_cast<uint8_t>(value.length() >> 8));
            recordOutput_->insert(recordOutput_->end(), value.c_str(), value.c_str() + value.length());
            return;
        }
        if (recordInput_) {
            if (recordInput_->size() - recordPosition_ < 2) { ok = false; return; }
            size_t size = (*recordInput_)[recordPosition_] | ((*recordInput_)[recordPosition_ + 1] << 8);
            recordPosition_ += 2;
            if (size > 30000 || size > recordInput_->size() - recordPosition_) { ok = false; return; }
            std::string decoded(reinterpret_cast<const char*>(recordInput_->data() + recordPosition_), size);
            recordPosition_ += size;
            if (decoded.find('\0') != std::string::npos) { ok = false; return; }
            value = decoded.c_str(); return;
        }
        const size_t expected = output_->length() + (output_->length() > 2 ? 2 : 0)
            + jsonLength(key) + 2 + jsonLength(value);
        if (output_->length() > 2) *output_ += ",\n";
        appendJsonString(*output_, key.c_str()); *output_ += ": "; appendJsonString(*output_, value.c_str());
        if (output_->length() != expected || expected > MaxConfigurationBackupLength) ok = false;
    }
    template<class T> void number(const String& key, T& value, uint32_t maximum) {
        String textValue = String(static_cast<unsigned int>(value));
        text(key, textValue);
        if (!ok || !reading()) return;
        uint32_t parsed = 0;
        if (textValue.length() == 0 || textValue.length() > 10) { ok = false; return; }
        for (size_t i = 0; i < textValue.length(); ++i) {
            char c = textValue[i];
            if (c < '0' || c > '9' || parsed > maximum / 10
                || (parsed == maximum / 10 && static_cast<unsigned>(c - '0') > maximum % 10)) { ok = false; return; }
            parsed = parsed * 10 + c - '0';
        }
        value = static_cast<T>(parsed);
    }
    void boolean(const String& key, bool& value) { number(key, value, 1); }
    void floating(const String& key, float& value) {
        char buffer[32]; snprintf(buffer, sizeof(buffer), "%.9g", static_cast<double>(value));
        String encoded(buffer); text(key, encoded);
        if (!ok) return;
        char* end = nullptr; float parsed = strtof(encoded.c_str(), &end);
        if (end == encoded.c_str() || *end || !std::isfinite(parsed)) { ok = false; return; }
        if (reading()) value = parsed;
    }
    void bytes(const String& key, uint8_t* value, size_t size) {
        static const char digits[] = "0123456789abcdef";
        String encoded;
        for (size_t i = 0; i < size; ++i) { encoded += digits[value[i] >> 4]; encoded += digits[value[i] & 15]; }
        text(key, encoded);
        if (!ok || !reading()) return;
        if (encoded.length() != size * 2) { ok = false; return; }
        for (size_t i = 0; i < size * 2; ++i) {
            const char* digit = strchr(digits, encoded[i]);
            if (!digit) { ok = false; return; }
            if (i % 2 == 0) value[i / 2] = static_cast<uint8_t>((digit - digits) << 4);
            else value[i / 2] |= static_cast<uint8_t>(digit - digits);
        }
    }
    void target(const String& prefix, ModuleActuatorReference& value) {
        bytes(prefix + ".instance", value.moduleInstanceFingerprint, 8);
        bytes(prefix + ".device", value.deviceIdHash, 4);
        bool instance = false, device = false;
        for (auto b : value.moduleInstanceFingerprint) instance |= b != 0;
        for (auto b : value.deviceIdHash) device |= b != 0;
        if (instance != device) ok = false;
    }
    void hardware(const String& prefix, HardwareResourceAssignment& hardware) {
        number(prefix + ".kind", hardware.kind, 2);
        number(prefix + ".gpio", hardware.gpio.number, 255);
        number(prefix + ".bus", hardware.i2c.bus, 1);
        number(prefix + ".address", hardware.i2c.address, 127);
    }
private:
    static size_t jsonLength(const String& text) {
        size_t length = 2;
        for (size_t i = 0; i < text.length(); ++i) {
            const unsigned char c = text[i];
            length += c == '"' || c == '\\' || c == '\b' || c == '\f' || c == '\n' || c == '\r' || c == '\t'
                ? 2 : c < 32 ? 6 : 1;
        }
        return length;
    }
    String* output_ = nullptr;
    JsonObject* input_ = nullptr;
    std::vector<uint8_t>* recordOutput_ = nullptr;
    const std::vector<uint8_t>* recordInput_ = nullptr;
    size_t recordPosition_ = 0;
};
void configurationFields(Fields& f, Configuration& c, bool passwords) {
    f.text("device.name", c.device.name);
    f.text("network.hostname", c.network.hostname);
    f.text("network.wifiSSID", c.network.wifiSSID);
    f.text("network.ipv4Address", c.network.ipv4Address);
    f.text("network.subnetMask", c.network.subnetMask);
    f.text("network.gateway", c.network.gateway);
    f.text("network.dns1", c.network.dns1);
    f.text("network.dns2", c.network.dns2);
    f.text("mqtt.server", c.mqtt.server);
    f.text("mqtt.username", c.mqtt.username);
    f.text("time.timezone", c.time.timezone);
    f.text("time.ntpServer1", c.time.ntpServer1);
    f.text("time.ntpServer2", c.time.ntpServer2);
    if (passwords) { f.text("network.wifiPassword", c.network.wifiPassword); f.text("mqtt.password", c.mqtt.password); }
    f.number("network.addressMode", c.network.addressMode, 1);
    f.number("mqtt.port", c.mqtt.port, 65535);
    f.number("locale.locale", c.locale.locale, 2);
    f.number("presentation.temperature", c.presentation.temperature, 255);
    f.number("presentation.atmosphericPressure", c.presentation.atmosphericPressure, 255);
    f.number("presentation.solarCellTemperature", c.presentation.solarCellTemperature, 255);
    f.number("presentation.rainDetectorLevel", c.presentation.rainDetectorLevel, 255);
    for (size_t i = 0; i < MaxSensorSlotCount; ++i) {
        auto& s = c.sensorSlots[i]; const String p = "sensor." + String(static_cast<unsigned int>(i + 1));
        f.number(p + ".id", s.slotId, MaxSensorSlotCount); f.boolean(p + ".enabled", s.enabled); f.text(p + ".name", s.name);
        const auto* metadata = SensorImplementationRegistry::find(s.implementation);
        String impl(metadata ? metadata->stableId : "invalid"); f.text(p + ".implementation", impl);
        metadata = SensorImplementationRegistry::findByStableId(impl.c_str());
        if (!metadata) { f.ok = false; return; } s.implementation = metadata->implementation;
        f.boolean(p + ".schedule.enabled", s.schedule.enabled);
        f.number(p + ".schedule.mode", s.schedule.acquisitionMode, 1);
        f.number(p + ".schedule.intervalMs", s.schedule.sampleIntervalMs, UINT32_MAX);
        f.hardware(p, s.hardware);
        auto& a = s.implementationConfiguration;
        f.number(p + ".am2302.gpio", a.am2302.gpio.number, 255);
        f.number(p + ".bme280.bus", a.bme280.i2c.bus, 1); f.number(p + ".bme280.address", a.bme280.i2c.address, 127);
        f.number(p + ".sht4x.bus", a.sht4x.i2c.bus, 1); f.number(p + ".sht4x.address", a.sht4x.i2c.address, 127);
        f.number(p + ".shtc3.bus", a.shtc3.i2c.bus, 1); f.number(p + ".shtc3.address", a.shtc3.i2c.address, 127);
        f.number(p + ".rain.gpio", a.rainGauge.gpio.number, 255);
        f.floating(p + ".rain.mmPerTip", a.rainGauge.millimetersPerTip);
        f.number(p + ".rain.debounceMs", a.rainGauge.debounceMs, UINT32_MAX);
    }
    for (size_t i = 0; i < MaxActuatorSlotCount; ++i) {
        auto& s = c.actuatorSlots[i]; const String p = "actuator." + String(static_cast<unsigned int>(i + 1));
        f.number(p + ".id", s.slotId, MaxActuatorSlotCount); f.boolean(p + ".enabled", s.enabled); f.text(p + ".name", s.name);
        const auto* metadata = ActuatorImplementationRegistry::find(s.implementation);
        String impl(metadata ? metadata->stableId : "invalid"); f.text(p + ".implementation", impl);
        metadata = ActuatorImplementationRegistry::findByStableId(impl.c_str());
        if (!metadata) { f.ok = false; return; } s.implementation = metadata->implementation;
        f.hardware(p, s.hardware); f.target(p + ".target", s.moduleTarget);
    }
    for (size_t i = 0; i < MaxControllerSlotCount; ++i) {
        auto& s = c.controllerSlots[i]; const String p = "controller." + String(static_cast<unsigned int>(i + 1));
        f.number(p + ".id", s.slotId, MaxControllerSlotCount); f.boolean(p + ".enabled", s.enabled); f.text(p + ".name", s.name);
        const auto* metadata = ControllerImplementationRegistry::find(s.implementation);
        String impl(metadata ? metadata->stableId : "invalid"); f.text(p + ".implementation", impl);
        metadata = ControllerImplementationRegistry::findByStableId(impl.c_str());
        if (!metadata) { f.ok = false; return; } s.implementation = metadata->implementation;
        f.target(p + ".target", s.moduleTarget);
        auto& b = s.implementationConfiguration.blink;
        f.number(p + ".blink.actuator", b.targetActuatorId, MaxActuatorSlotCount);
        f.number(p + ".blink.onMs", b.onDurationMs, UINT32_MAX); f.number(p + ".blink.offMs", b.offDurationMs, UINT32_MAX);
        auto& t = s.implementationConfiguration.threshold;
        f.boolean(p + ".threshold.decisionOnly", t.decisionOnly);
        f.number(p + ".threshold.sensor", t.source.sensorId, MaxSensorSlotCount);
        String type(measurementTypeStableId(t.source.measurementType)); f.text(p + ".threshold.measurement", type);
        t.source.measurementType = measurementTypeFromStableId(type.c_str());
        if (type != measurementTypeStableId(t.source.measurementType)) f.ok = false;
        f.number(p + ".threshold.actuator", t.targetActuatorId, MaxActuatorSlotCount);
        f.floating(p + ".threshold.on", t.onThreshold); f.floating(p + ".threshold.off", t.offThreshold);
        f.number(p + ".threshold.direction", t.direction, 1); f.number(p + ".threshold.maxAgeMs", t.maxMeasurementAgeMs, UINT32_MAX);
        auto& sel = s.implementationConfiguration.selector;
        f.number(p + ".selector.value", sel.modeValueId, 65535);
        f.number(p + ".selector.controller", sel.automaticControllerId, MaxControllerSlotCount);
        f.number(p + ".selector.actuator", sel.targetActuatorId, MaxActuatorSlotCount);
        f.text(p + ".selector.automatic", sel.automaticCode); f.text(p + ".selector.on", sel.onCode); f.text(p + ".selector.off", sel.offCode);
    }
    String display, values;
    if (!encodeDisplayConfiguration(c.display, display) || !encodeValueConfiguration(c.values, values)) { f.ok = false; return; }
    f.text("display.record", display); f.text("values.record", values);
    f.boolean("display.configured", c.display.configured);
    if (f.reading()) {
        const bool configured = c.display.configured;
        if (!decodeDisplayConfiguration(display, c.display) || !decodeValueConfiguration(values, c.values)) f.ok = false;
        c.display.configured = configured;
    }
}
void header(Fields& f, bool& passwords, String& firmware) {
    String format("EnvNode-configuration"), version("1");
    String license("LicenseRef-EnvNode-Use-and-Redistribution-1.0; see LICENSE");
    unsigned int profile = static_cast<unsigned int>(currentBoardProfile().id);
    f.text("format", format); f.text("version", version); f.text("license", license);
    f.text("firmware", firmware); f.number("boardProfile", profile, 255); f.boolean("includesPasswords", passwords);
    if (format != "EnvNode-configuration" || version != "1"
        || profile != static_cast<unsigned int>(currentBoardProfile().id)) f.ok = false;
}
}
bool encodeConfigurationBackup(const Configuration& configuration, bool includePasswords,
    const char* firmwareVersion, String& output) {
    std::unique_ptr<Configuration> copy(new (std::nothrow) Configuration(configuration));
    if (!copy) return false;
    String result("{\n"), firmware(firmwareVersion);
    Fields f(result); header(f, includePasswords, firmware); configurationFields(f, *copy, includePasswords);
    const size_t expected = result.length() + 3;
    result += "\n}\n";
    if (!f.ok || result.length() != expected || result.length() > MaxConfigurationBackupLength) return false;
    output = std::move(result); return output.length() == expected;
}
bool decodeConfigurationBackup(const String& input, const Configuration& current,
    Configuration& candidate, bool& includesPasswords) {
    if (input.length() > MaxConfigurationBackupLength || strlen(input.c_str()) != input.length()) return false;
    JsonObject object(input);
    if (!object.parse()) return false;
    std::unique_ptr<Configuration> copy(new (std::nothrow) Configuration(current));
    if (!copy) return false;
    bool passwords = false; String firmware;
    Fields f(object); header(f, passwords, firmware); configurationFields(f, *copy, passwords);
    if (!f.ok || !object.empty()) return false;
    candidate = *copy; includesPasswords = passwords; return true;
}
bool encodeConfigurationRecord(const Configuration& configuration, std::vector<uint8_t>& output) {
    std::unique_ptr<Configuration> copy(new (std::nothrow) Configuration(configuration));
    if (!copy) return false;
    std::vector<uint8_t> result;
    Fields f(result, true); bool passwords = true; String firmware;
    header(f, passwords, firmware); configurationFields(f, *copy, true);
    if (!f.ok || result.size() > MaxConfigurationBackupLength) return false;
    output.swap(result); return true;
}
bool decodeConfigurationRecord(const std::vector<uint8_t>& input, const Configuration& current,
    Configuration& candidate) {
    if (input.size() > MaxConfigurationBackupLength) return false;
    std::unique_ptr<Configuration> copy(new (std::nothrow) Configuration(current));
    if (!copy) return false;
    Fields f(input); bool passwords = true; String firmware;
    header(f, passwords, firmware); configurationFields(f, *copy, passwords);
    if (!f.ok || !f.complete() || !passwords) return false;
    candidate = *copy; return true;
}

}
