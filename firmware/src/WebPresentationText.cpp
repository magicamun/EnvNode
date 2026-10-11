// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#include "WebPresentationText.h"
#include "UnitConverter.h"
namespace EnvNode {
const char* webMeasurementName(Locale locale, MeasurementType value) {
    switch (value) {
        case MeasurementType::Temperature: return webText(locale, TextId::Temperature);
        case MeasurementType::RelativeHumidity: return webText(locale, TextId::RelativeHumidity);
        case MeasurementType::AtmosphericPressure: return webText(locale, TextId::AtmosphericPressure);
        case MeasurementType::SolarIrradiance: return webText(locale, TextId::SolarIrradiance);
        case MeasurementType::SolarCellTemperature: return webText(locale, TextId::SolarCellTemperature);
        case MeasurementType::RainDetectorLevel: return webText(locale, TextId::RainDetectorLevel);
        case MeasurementType::RainDetectorWet: return webText(locale, TextId::RainDetectorWet);
        case MeasurementType::RainGaugeTip: return webText(locale, TextId::RainGaugeTip);
        case MeasurementType::RainfallIncrement: return webText(locale, TextId::RainfallIncrement);
        case MeasurementType::HydrostaticPressure: return webText(locale, TextId::HydrostaticPressure);
        case MeasurementType::WaterLevel: return webText(locale, TextId::WaterLevel);
        default: return webText(locale, TextId::Unknown);
    }
}
const char* webQualityName(Locale locale, MeasurementQuality value) {
    switch (value) {
        case MeasurementQuality::Good: return webText(locale, TextId::Good);
        case MeasurementQuality::Estimated: return webText(locale, TextId::Estimated);
        case MeasurementQuality::Degraded: return webText(locale, TextId::Degraded);
        case MeasurementQuality::BelowMeasurementRange: return webText(locale, TextId::BelowMeasurementRange);
        default: return webText(locale, TextId::Unknown);
    }
}
const char* webUnitName(Locale locale, PresentationUnit value) {
    switch (value) {
        case PresentationUnit::DegreeCelsius: return webText(locale, TextId::Celsius);
        case PresentationUnit::DegreeFahrenheit: return webText(locale, TextId::Fahrenheit);
        case PresentationUnit::Pascal: return webText(locale, TextId::Pascal);
        case PresentationUnit::Percent: return webText(locale, TextId::Percent);
        case PresentationUnit::WattPerSquareMetre: return webText(locale, TextId::WattPerSquareMetre);
        case PresentationUnit::Ratio: return webText(locale, TextId::Ratio);
        case PresentationUnit::Millimeter: return webText(locale, TextId::Millimetre);
        case PresentationUnit::Metre: return webText(locale, TextId::Metre);
        case PresentationUnit::Centimetre: return webText(locale, TextId::Centimetre);
        case PresentationUnit::None: return webText(locale, TextId::None);
        default: return UnitConverter::displayName(value);
    }
}
const char* webFormatError(Locale locale, PropertyFormatStatus value) {
    if (value == PropertyFormatStatus::Formatted) return "";
    switch (value) {
        case PropertyFormatStatus::InvalidFormat: return webText(locale, TextId::UseUpToFourFUOrS);
        case PropertyFormatStatus::FormatTooLong: return webText(locale, TextId::FormatIsTooLongMaximum128Bytes);
        case PropertyFormatStatus::TypeMismatch: return webText(locale, TextId::FormatDoesNotMatchTheSourceTypeF);
        case PropertyFormatStatus::UnknownReference: return webText(locale, TextId::SourceIsUnknownOrNoLongerAvailable);
        case PropertyFormatStatus::NoValue: return webText(locale, TextId::NoValueAvailableYet);
        case PropertyFormatStatus::InvalidValue: return webText(locale, TextId::TheCurrentSourceValueIsInvalid);
        case PropertyFormatStatus::SourceCountMismatch: return webText(locale, TextId::TheNumberOfSourcesMustMatchThePlaceholders);
        case PropertyFormatStatus::OutputTooLong: return webText(locale, TextId::FormattedLineIsTooLongMaximum128Bytes);
        default: return webText(locale, TextId::UnableToFormatThisValue);
    }
}
const char* webDisplayLineError(Locale locale, const DisplayLineResult& result) {
    switch (result.issue) {
        case DisplayLineIssue::InvalidLine: return webText(locale, TextId::InvalidLine);
        case DisplayLineIssue::BooleanWithoutSource: return webText(locale, TextId::BooleanTextRequiresASource);
        case DisplayLineIssue::SourceGap: return webText(locale, TextId::FillSourcesInOrderWithoutGaps);
        case DisplayLineIssue::InvalidSource: return webText(locale, TextId::InvalidSourceUseCategoryIDProperty);
        case DisplayLineIssue::InvalidBooleanText: return webText(locale, TextId::BooleanTextMaximum16UTF8BytesNo);
        case DisplayLineIssue::TooManyTranslations: return webText(locale, TextId::TooManyStateTranslations);
        case DisplayLineIssue::FormatError: return webFormatError(locale, result.value.status);
        case DisplayLineIssue::None: return nullptr;
    }
    return nullptr;
}
const char* webSensorImplementationName(Locale locale, SensorImplementation value) {
    switch (value) {
        case SensorImplementation::None: return webText(locale, TextId::None);
        case SensorImplementation::SimulatedTemperature: return webText(locale, TextId::SimulatedTemperature);
        case SensorImplementation::SimulatedHumidity: return webText(locale, TextId::SimulatedHumidity);
        case SensorImplementation::SimulatedPressure: return webText(locale, TextId::SimulatedPressure);
        case SensorImplementation::RainGauge: return webText(locale, TextId::RainGauge);
        default: {
            const auto* metadata = SensorImplementationRegistry::find(value);
            return metadata ? metadata->displayType : webText(locale, TextId::Unknown);
        }
    }
}
const char* webActuatorImplementationName(Locale locale, ActuatorImplementation value) {
    switch (value) {
        case ActuatorImplementation::None: return webText(locale, TextId::None);
        case ActuatorImplementation::GpioOnOff: return webText(locale, TextId::GPIOOnOff);
        case ActuatorImplementation::GpioPwm: return webText(locale, TextId::GPIOPWMLevel);
        default: {
            const auto* metadata = ActuatorImplementationRegistry::find(value);
            return metadata ? metadata->displayType : webText(locale, TextId::Unknown);
        }
    }
}
const char* webControllerImplementationName(Locale locale, ControllerImplementation value) {
    switch (value) {
        case ControllerImplementation::None: return webText(locale, TextId::None);
        case ControllerImplementation::Blink: return webText(locale, TextId::Blink);
        case ControllerImplementation::Threshold: return webText(locale, TextId::ThresholdHysteresis);
        case ControllerImplementation::Selector: return webText(locale, TextId::Selector);
        default: {
            const auto* metadata = ControllerImplementationRegistry::find(value);
            return metadata ? metadata->displayType : webText(locale, TextId::Unknown);
        }
    }
}
const char* webSensorDescription(Locale locale, SensorImplementation value) {
    switch (value) {
        case SensorImplementation::None: return webText(locale, TextId::NoRuntimeSensor2);
        case SensorImplementation::SimulatedTemperature: return webText(locale, TextId::FirmwareSimulation);
        case SensorImplementation::SimulatedHumidity: return webText(locale, TextId::FirmwareSimulation);
        case SensorImplementation::SimulatedPressure: return webText(locale, TextId::FirmwareSimulation);
        case SensorImplementation::AM2302: return webText(locale, TextId::CustomSingleWireProtocol);
        case SensorImplementation::RainGauge: return webText(locale, TextId::DigitalInterrupt);
        default: {
            const auto* metadata = SensorImplementationRegistry::find(value);
            return metadata ? metadata->protocolDescription : webText(locale, TextId::Unknown);
        }
    }
}
const char* webActuatorDescription(Locale locale, ActuatorImplementation value) {
    switch (value) {
        case ActuatorImplementation::None: return webText(locale, TextId::NoRuntimeActuator2);
        case ActuatorImplementation::GpioOnOff: return webText(locale, TextId::DigitalOutput);
        case ActuatorImplementation::GpioPwm: return webText(locale, TextId::PWMOutput);
        default: {
            const auto* metadata = ActuatorImplementationRegistry::find(value);
            return metadata ? metadata->protocolDescription : webText(locale, TextId::Unknown);
        }
    }
}
const char* webControllerDescription(Locale locale, ControllerImplementation value) {
    switch (value) {
        case ControllerImplementation::None: return webText(locale, TextId::NoRuntimeController2);
        case ControllerImplementation::Blink: return webText(locale, TextId::PeriodicallySwitchesAnOnOffActuator);
        case ControllerImplementation::Threshold: return webText(locale, TextId::ControlsAnOnOffActuatorFromNumericMeasurement);
        case ControllerImplementation::Selector: return webText(locale, TextId::SelectsAutomaticDecisionOrFixedOnOffFrom);
        default: {
            const auto* metadata = ControllerImplementationRegistry::find(value);
            return metadata ? metadata->description : webText(locale, TextId::Unknown);
        }
    }
}
} // namespace EnvNode
