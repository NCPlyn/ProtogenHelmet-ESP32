//--------------------------------//realtime logger

#pragma once

#define LOG_BUFFER_SIZE (50 * 1024)  // 50 KB

char * logBuffer;
size_t logIndex;

void logPrint(const char * str);

inline void logPrint(const __FlashStringHelper *str) {
  logPrint((const char*)str);
}

inline void logPrint(const String &str) {
  logPrint(str.c_str());
}
