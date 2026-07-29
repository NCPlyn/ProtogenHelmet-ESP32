#pragma once
#include "settings.h"
#include "animStructs.h"

//--------------------------------//Config vars
extern bool instantReload, oledInitDone, tiltInitDone, ToFInitDone;
extern uint8_t currentEarsFrame, currentVisorFrame, numOfSegm, numAnimBlush,totalAnims;
constexpr uint16_t visorLedsNum = MATRIXESNUM*64;
extern String currentAnim, animToLoad, availAnims[50];
extern float micDC;

//--------------------------------//Structs for anims in psram
extern AnimNowEars* earsNow;
extern AnimNowVisor* visorNow;
