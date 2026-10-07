#pragma once

#include <Arduino.h>

// Standalone bench defaults; adjust to the actual wiring before flashing.
namespace LabConfig {
constexpr uint8_t SdaPin = 21;
constexpr uint8_t SclPin = 22;
constexpr uint8_t Address = 0x3C; // Seven-bit scanner address.
constexpr uint32_t BusClockHz = 100000;
constexpr uint32_t SerialBaud = 115200;
constexpr uint8_t LineHeight = 10;
}
