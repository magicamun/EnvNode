// EnvNode: see repository LICENSE.
#pragma once
#include <Arduino.h>
namespace EnvNode {
enum class TextUploadResult { Success, InvalidSize, TooLarge, OutOfMemory };
TextUploadResult parseTextUploadSize(const String& hint, size_t maximum, size_t& size);
TextUploadResult prepareTextUpload(String& text, const String& hint, size_t maximum);
TextUploadResult appendTextUpload(String& text, const uint8_t* data, size_t size, size_t maximum);
const char* textUploadResultName(TextUploadResult result);
}
