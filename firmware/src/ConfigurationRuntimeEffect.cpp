#include "ConfigurationRuntimeEffect.h"
#include "SensorSlotConfiguration.h"
#include "ActuatorSlotConfiguration.h"
#include "ControllerSlotConfiguration.h"

namespace EnvNode {

namespace {
bool sameHardware(const HardwareResourceAssignment& a,const HardwareResourceAssignment& b) {
    if(a.kind!=b.kind)return false;
    switch(a.kind) {
        case HardwareResourceKind::None:return true;
        case HardwareResourceKind::GPIO:return a.gpio.number==b.gpio.number;
        case HardwareResourceKind::I2C:return a.i2c.bus==b.i2c.bus && a.i2c.address==b.i2c.address;
        default:return false;
    }
}
bool sameModule(const ModuleActuatorReference& a,const ModuleActuatorReference& b) {
    return !memcmp(a.moduleInstanceFingerprint,b.moduleInstanceFingerprint,sizeof(a.moduleInstanceFingerprint))
        && !memcmp(a.deviceIdHash,b.deviceIdHash,sizeof(a.deviceIdHash));
}
bool sameSensor(const SensorSlotConfiguration& a,const SensorSlotConfiguration& b) {
    if(a.slotId!=b.slotId || a.enabled!=b.enabled || a.name!=b.name || a.implementation!=b.implementation
        || a.schedule.enabled!=b.schedule.enabled || a.schedule.acquisitionMode!=b.schedule.acquisitionMode
        || a.schedule.sampleIntervalMs!=b.schedule.sampleIntervalMs || !sameHardware(a.hardware,b.hardware))return false;
    const auto& x=a.implementationConfiguration;const auto& y=b.implementationConfiguration;
    switch(a.implementation) {
        case SensorImplementation::RainGauge:return x.rainGauge.gpio.number==y.rainGauge.gpio.number
            && x.rainGauge.millimetersPerTip==y.rainGauge.millimetersPerTip && x.rainGauge.debounceMs==y.rainGauge.debounceMs;
        case SensorImplementation::AM2302:return x.am2302.gpio.number==y.am2302.gpio.number;
        case SensorImplementation::BME280:return x.bme280.i2c.bus==y.bme280.i2c.bus && x.bme280.i2c.address==y.bme280.i2c.address;
        case SensorImplementation::SHT4x:return x.sht4x.i2c.bus==y.sht4x.i2c.bus && x.sht4x.i2c.address==y.sht4x.i2c.address;
        case SensorImplementation::SHTC3:return x.shtc3.i2c.bus==y.shtc3.i2c.bus && x.shtc3.i2c.address==y.shtc3.i2c.address;
        case SensorImplementation::None:case SensorImplementation::SimulatedTemperature:
        case SensorImplementation::SimulatedHumidity:case SensorImplementation::SimulatedPressure:
        case SensorImplementation::SolarPosition:case SensorImplementation::RainDetector:return true;
        default:return false; // new implementations must declare their comparison
    }
}
bool sameController(const ControllerSlotConfiguration& a,const ControllerSlotConfiguration& b) {
    if(a.slotId!=b.slotId || a.enabled!=b.enabled || a.name!=b.name || a.implementation!=b.implementation
        || !sameModule(a.moduleTarget,b.moduleTarget))return false;
    const auto& x=a.implementationConfiguration;const auto& y=b.implementationConfiguration;
    switch(a.implementation) {
        case ControllerImplementation::None:return true;
        case ControllerImplementation::Blink:return x.blink.targetActuatorId==y.blink.targetActuatorId
            && x.blink.onDurationMs==y.blink.onDurationMs && x.blink.offDurationMs==y.blink.offDurationMs;
        case ControllerImplementation::Threshold:return x.threshold.decisionOnly==y.threshold.decisionOnly
            && x.threshold.source==y.threshold.source && x.threshold.targetActuatorId==y.threshold.targetActuatorId
            && x.threshold.onThreshold==y.threshold.onThreshold && x.threshold.offThreshold==y.threshold.offThreshold
            && x.threshold.direction==y.threshold.direction && x.threshold.maxMeasurementAgeMs==y.threshold.maxMeasurementAgeMs;
        case ControllerImplementation::Selector:return x.selector.modeValueId==y.selector.modeValueId
            && x.selector.automaticControllerId==y.selector.automaticControllerId && x.selector.targetActuatorId==y.selector.targetActuatorId
            && x.selector.automaticCode==y.selector.automaticCode && x.selector.onCode==y.selector.onCode && x.selector.offCode==y.selector.offCode;
        case ControllerImplementation::RainDetector: {
            const auto& r=x.rainDetector;const auto& s=y.rainDetector;const auto& p=r.policy;const auto& q=s.policy;
            return r.sourceSensorId==s.sourceSensorId && r.targetActuatorId==s.targetActuatorId
                && r.temperatureSource==s.temperatureSource && r.humiditySource==s.humiditySource
                && p.edgeMs==q.edgeMs && p.dropHz==q.dropHz && p.dropPermille==q.dropPermille
                && p.endSeconds==q.endSeconds && p.dryHz==q.dryHz && p.hysteresisHz==q.hysteresisHz && p.drySeconds==q.drySeconds;
        }
        default:return false;
    }
}
}
RuntimeAction runtimeActionForChange(const SensorSlotConfiguration& before,const SensorSlotConfiguration& after) {
    return sameSensor(before,after)?RuntimeAction::None:RuntimeAction::RestartSensorManager;
}
RuntimeAction runtimeActionForChange(const ActuatorSlotConfiguration& a,const ActuatorSlotConfiguration& b) {
    return a.slotId==b.slotId && a.enabled==b.enabled && a.name==b.name && a.implementation==b.implementation
        && sameHardware(a.hardware,b.hardware) && sameModule(a.moduleTarget,b.moduleTarget)?RuntimeAction::None:RuntimeAction::RestartActuatorRuntime;
}
RuntimeAction runtimeActionForChange(const ControllerSlotConfiguration& before,const ControllerSlotConfiguration& after) {
    return sameController(before,after)?RuntimeAction::None:RuntimeAction::RestartControllerRuntime;
}

RuntimeAction runtimeActionFor(ConfigurationArea area) {
    switch (area) {
        case ConfigurationArea::Network: return RuntimeAction::RestartDevice;
        case ConfigurationArea::Mqtt: return RuntimeAction::RestartMqtt;
        case ConfigurationArea::Time: return RuntimeAction::RestartTime;
        case ConfigurationArea::Locale: return RuntimeAction::None;
        case ConfigurationArea::PresentationUnits: return RuntimeAction::None;
        case ConfigurationArea::Device:
            // The device name contributes to the MQTT client identity on connect.
            return RuntimeAction::RestartMqtt;
        case ConfigurationArea::GeoPosition:
        case ConfigurationArea::Sensors: return RuntimeAction::RestartSensorManager;
        case ConfigurationArea::Actuators: return RuntimeAction::RestartActuatorRuntime;
        case ConfigurationArea::Controllers: return RuntimeAction::RestartControllerRuntime;
        default: return RuntimeAction::None;
    }
}

ConfigurationSaveResult configurationSaveResult(bool success, ConfigurationArea area) {
    return {success, success ? runtimeActionFor(area) : RuntimeAction::None};
}

ConfigurationSaveResult configurationSaveResult(
    bool success,
    ConfigurationArea firstArea,
    bool firstChanged,
    ConfigurationArea secondArea,
    bool secondChanged) {
    if (!success) return {false, RuntimeAction::None};
    const RuntimeAction firstAction = firstChanged
        ? runtimeActionFor(firstArea) : RuntimeAction::None;
    const RuntimeAction secondAction = secondChanged
        ? runtimeActionFor(secondArea) : RuntimeAction::None;
    return {
        true,
        static_cast<uint8_t>(firstAction) >= static_cast<uint8_t>(secondAction)
            ? firstAction : secondAction,
    };
}

} // namespace EnvNode
