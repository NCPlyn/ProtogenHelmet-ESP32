#include "loadFunctions.h"
#include "logger.h"
#include <LittleFS.h>
#include <StreamUtils.h>
#include "fileOp.h"
#include "oled.h"
#include <stdlib.h>
#include <string.h>

bool loadAnim(const String & anim, const String & temp) {
  JsonDocument doc;
  DeserializationError error;

  if(anim == "POSTAnimLoad") {
    logPrint(F("[I] POST load"));
    error = deserializeJson(doc, temp);
  } else if(currentAnim == anim) {
    return false;
  } else {
    delay(25);
    File file = LittleFS.open("/anims/"+anim, "r");
    if (!file) {
      logPrint(F("[E] There was an error opening the animation file!"));
      file.close();
      return false;
    }
    logPrint(F("[I] Animation file opened!"));
    ReadBufferingStream bufferedFile{file, 64};
    error = deserializeJson(doc, bufferedFile);
    file.close();
  }

  if(error){
    logPrint("[E] Failed to deserialize animation file! : " + String(error.c_str()));
    return false;
  }

  // Free previously allocated frames
  animNowFramesRelease();

  currentAnim = anim;

  animNowTypesLoad(doc); // Ears and Visor types load

  //Ears anim load
  earsNow->numOfFrames = doc["ears"]["frames"].size();
  earsNow->frames = (FramesEars*) ps_malloc(sizeof(FramesEars) * earsNow->numOfFrames);
  for(int x = 0; x < earsNow->numOfFrames; x++) { earsNowFrameLoad(doc, x); }
  //isMouth
  for(int y = 0; y < doc["visor"]["isMouth"].size(); y++) {
    visorNow->isMouth[y] = doc["visor"]["isMouth"][y].as<bool>();
  }
  //Visor anim load
  visorNow->numOfFrames = doc["visor"]["frames"].size();
  visorNow->frames = (FramesVisor*) ps_malloc(sizeof(FramesVisor) * visorNow->numOfFrames);
  for(int x = 0; x < visorNow->numOfFrames; x++) { visorNowFrameLoad(doc, x); }

  instantReload = true;
  currentVisorFrame = 0;
  currentEarsFrame = 0;

  if(cfg.oledEna && oledInitDone) {
    oled.writeAnim(anim.substring(0,anim.length()-5));
    oled.writeRGB(vTAcro[visorNow->type]);
  }
  return true;
}

void animNowTypesLoad(const JsonDocument & doc)
{
  //Ears anim type
  for(int o=0;o<earTypeSize;o++) {
    if(doc["ears"]["type"].as<String>() != earTypes[o]) continue;
    earsNow->type = o;
    break;
  }
  //Visor anim type
  for(int o=0;o<visTypeSize;o++) {
    if(doc["visor"]["type"].as<String>() != visorTypes[o]) continue;
    visorNow->type = o;
    break;
  }
}

void earsNowFrameLoad(const JsonDocument & doc, int x)
{
    earsNow->frames[x].timespan = doc["ears"]["frames"][x]["timespan"].as<int>();
    for(int y = 0; y < doc["ears"]["frames"][x]["leds"].size(); y++) {
      earsNow->frames[x].ledColor[y] = strtol(doc["ears"]["frames"][x]["leds"][y].as<const char *>(), NULL, 16);
    }
}

void visorNowFrameLoad(const JsonDocument & doc, int x)
{
  visorNow->frames[x].timespan = doc["visor"]["frames"][x]["timespan"].as<int>();
  numAnimBlush = (uint8_t)doc["visor"]["frames"][x]["ledsBlush"].size();
  for(int y = 0; y < numAnimBlush; y++) {
    if(y < blushLedsNum)
      visorNow->frames[x].ledsBlush[y] = strtol(doc["visor"]["frames"][x]["ledsBlush"][y].as<const char *>(), NULL, 16);
  }
  numOfSegm = doc["visor"]["frames"][x]["leds"].size();
  for(int y = 0; y < numOfSegm; y++) {
    visorNow->frames[x].fColor[y] = strtol(doc["visor"]["frames"][x]["fColor"][y].as<const char *>(), NULL, 16); //should return 0 if not present
    visorNow->frames[x].leds[y] = strtoull(doc["visor"]["frames"][x]["leds"][y].as<const char *>(), NULL, 16); //string to uint64
  }

  memset(visorNow->frames[x]->ppColor, 0, sizeof(visorNow->frames[x]->ppColor)); //wipe ppColor data

  int numOfpp = doc["visor"]["frames"][x]["ppColor"].size();
  for(int y = 0; y < numOfpp; y++) {
    int numOfppData = doc["visor"]["frames"][x]["ppColor"][y]["data"].size();
    for(int z = 0; z < numOfppData; z++) { //ppColor[matrix][pixel] = color
      visorNow->frames[x].ppColor[doc["visor"]["frames"][x]["ppColor"][y]["mIndex"].as<int>()][doc["visor"]["frames"][x]["ppColor"][y]["data"][z][0].as<int>()] = strtol(doc["visor"]["frames"][x]["ppColor"][y]["data"][z][1].as<const char *>(), NULL, 16);
    }
  }
}
