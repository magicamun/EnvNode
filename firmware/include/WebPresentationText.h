// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#pragma once
#include "WebLocalization.h"
#include "MeasurementType.h"
#include "MeasurementQuality.h"
#include "PresentationUnit.h"
#include "PropertyTextFormatter.h"
#include "DisplayPageFormatter.h"
#include "SensorImplementationRegistry.h"
#include "ActuatorImplementationRegistry.h"
#include "ControllerImplementationRegistry.h"
namespace EnvNode {
const char* webMeasurementName(Locale locale, MeasurementType value);
const char* webQualityName(Locale locale, MeasurementQuality value);
const char* webUnitName(Locale locale, PresentationUnit value);
const char* webFormatError(Locale locale, PropertyFormatStatus value);
const char* webDisplayLineError(Locale locale, const DisplayLineResult& result);
const char* webSensorImplementationName(Locale locale, SensorImplementation value);
const char* webActuatorImplementationName(Locale locale, ActuatorImplementation value);
const char* webControllerImplementationName(Locale locale, ControllerImplementation value);
const char* webSensorDescription(Locale locale, SensorImplementation value);
const char* webActuatorDescription(Locale locale, ActuatorImplementation value);
const char* webControllerDescription(Locale locale, ControllerImplementation value);
} // namespace EnvNode
