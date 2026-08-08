#pragma once

#include <stdint.h>

namespace WeatherStation {

enum class PresentationUnit : uint8_t {
    None = 0,
    DegreeCelsius = 1,
    DegreeFahrenheit = 2,
    Pascal = 3,
    Hectopascal = 4,
    Kilopascal = 5,
    InchMercury = 6,
    Percent = 7,
    WattPerSquareMetre = 8,
    Ratio = 9,
    Millimeter = 10,
};

} // namespace WeatherStation
