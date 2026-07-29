#pragma once

#include <Arduino.h>
#include <FastLED.h>

void setAllVisor(struct CRGB *ledArray, long ledColor, int visorFrame);
void dynamicSpeak(uint64_t *leds, bool isMouth[MATRIXESNUM], int volume);
