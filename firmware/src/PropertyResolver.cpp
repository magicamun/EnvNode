#include "PropertyResolver.h"

#include "SensorManager.h"
#include "ActuatorRuntime.h"
#include "ControllerRuntime.h"
#include "SensorPropertyReader.h"
#include "ActuatorPropertyReader.h"
#include "ControllerPropertyReader.h"

namespace EnvNode {

PropertyResolver::PropertyResolver(const SensorManager& sensors, const IMeasurementResolver& measurements,
    const ActuatorRuntime& actuators, const ControllerRuntime& controllers)
    : sensors_(sensors), measurements_(measurements), actuators_(actuators), controllers_(controllers) {}

bool PropertyResolver::describe(const PropertyReference& reference, PropertyDescription& result) const {
    result = PropertyDescription{};
    switch (reference.componentKind) {
        case PropertyComponentKind::Sensor: {
            const ISensor* sensor = sensors_.sensor(reference.componentId);
            if (sensor == nullptr) return false;
            return SensorPropertyReader(*sensor, measurements_).describe(reference, result);
        }
        case PropertyComponentKind::Actuator: {
            const IOnOffActuator* actuator = actuators_.onOffActuator(reference.componentId);
            if (actuator == nullptr) return false;
            return ActuatorPropertyReader(reference.componentId, *actuator).describe(reference, result);
        }
        case PropertyComponentKind::Controller: {
            const IThresholdReasonProvider* provider = controllers_.reasonProvider(reference.componentId);
            if (provider == nullptr) return false;
            return ControllerPropertyReader(reference.componentId, *provider).describe(reference, result);
        }
        default: return false;
    }
}

PropertyReadResult PropertyResolver::read(const PropertyReference& reference, PropertySnapshot& result) const {
    result = PropertySnapshot{};
    switch (reference.componentKind) {
        case PropertyComponentKind::Sensor: {
            const ISensor* sensor = sensors_.sensor(reference.componentId);
            if (sensor == nullptr) return PropertyReadResult::UnknownReference;
            return SensorPropertyReader(*sensor, measurements_).read(reference, result);
        }
        case PropertyComponentKind::Actuator: {
            const IOnOffActuator* actuator = actuators_.onOffActuator(reference.componentId);
            if (actuator == nullptr) return PropertyReadResult::UnknownReference;
            return ActuatorPropertyReader(reference.componentId, *actuator).read(reference, result);
        }
        case PropertyComponentKind::Controller: {
            const IThresholdReasonProvider* provider = controllers_.reasonProvider(reference.componentId);
            if (provider == nullptr) return PropertyReadResult::UnknownReference;
            return ControllerPropertyReader(reference.componentId, *provider).read(reference, result);
        }
        default: return PropertyReadResult::UnknownReference;
    }
}

} // namespace EnvNode
