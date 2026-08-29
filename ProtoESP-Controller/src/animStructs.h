#pragma once
#include "settings.h"
#include <Arduino.h>
#include <stdint.h>

//--------------------------------//Structs for anims in psram
struct FramesEars {
  int timespan;
  long ledColor[earLedsNum];
};

struct AnimNowEars {
  int type;
  int numOfFrames;
  FramesEars* frames = nullptr;
};

struct FramesVisor {
  int timespan;
  uint64_t leds[MATRIXESNUM];
  long ledsBlush[blushLedsNum];
  long fColor[MATRIXESNUM];
  long ppColor[MATRIXESNUM][64];
};

struct AnimNowVisor {
  int type;
  int numOfFrames;
  FramesVisor* frames = nullptr;
  bool isMouth[MATRIXESNUM];
};
