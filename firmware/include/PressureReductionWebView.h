// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#pragma once
#include "Configuration.h"
namespace EnvNode {
String buildPressureReductionConfigurationHtml(const Configuration& configuration, Locale locale);
String buildPressureReductionDiagnosticHtml(const PressureReductionStatus& status, Locale locale);
}
