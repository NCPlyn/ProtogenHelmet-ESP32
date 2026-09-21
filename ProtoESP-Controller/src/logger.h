#pragma once
#include <Arduino.h>
//--------------------------------//realtime logger

#define LOG_BUFFER_SIZE (50 * 1024)  // 50 KB

extern char * logBuffer;
extern size_t logIndex;

void logPrint(const char * str);

inline void logPrint(const __FlashStringHelper *str) {
  logPrint((const char*)str);
}

inline void logPrint(const String &str) {
  logPrint(str.c_str());
}
