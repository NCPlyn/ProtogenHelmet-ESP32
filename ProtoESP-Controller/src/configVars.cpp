#include "configVars.h"
#include <Arduino.h>

//--------------------------------//Config vars
bool instantReload = false, oledInitDone = false, tiltInitDone = false, ToFInitDone = false;
uint8_t currentEarsFrame = 0, currentVisorFrame = 0, numOfSegm = 0, numAnimBlush = 0, totalAnims = 0;
String currentAnim = "", animToLoad = "", availAnims[50];
float micDC = 800;
