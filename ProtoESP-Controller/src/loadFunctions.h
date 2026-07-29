#pragma once
#include <Arduino.h>
#include "configVars.h"

//--------------------------------//Load functions
inline void animNowFramesRelease() // Free previously allocated frames
{
  if (earsNow->frames) { free(earsNow->frames); earsNow->frames = nullptr; }
  if (visorNow->frames) { free(visorNow->frames); visorNow->frames = nullptr; }
}

bool loadAnim(const String &, const String &);

void animNowTypesLoad(const JsonDocument &);

void earsNowFrameLoad(const JsonDocument &, int);

void visorNowFrameLoad(const JsonDocument &, int);
