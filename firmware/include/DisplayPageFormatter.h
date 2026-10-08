#pragma once
#include "DisplayConfiguration.h"
namespace EnvNode {
struct DisplayLineResult {
    PropertyTextResult value;
    const char* error = nullptr;
};
DisplayLineResult formatDisplayLine(const IPropertyReader& reader,
    const DisplayPage& page, size_t line);
}
