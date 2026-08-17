#pragma once

#include <Arduino.h>

#include "IRecentLogReader.h"

namespace EnvNode {

String buildRecentLogHtml(const IRecentLogReader& logReader);

} // namespace EnvNode
