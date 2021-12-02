//I'M NEVER EVER FCKING TOUCHING THIS CODE AGAIN

//DOIT ESP32 DEVKIT V1
// L5,18,23 - MATRIX done
// L16,17(blush) - WS2812  done
// B35 - mic done
// B33(z),32(y) - gyro done
// B27 - touch (future another with 26)

#include <StreamUtils.h>
#include <sstream>
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include <ArduinoJson.h>

#include "WiFi.h"
#include "ESPAsyncWebServer.h"

#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#define CONFIG_LITTLEFS_CACHE_SIZE 256
#define SPIFFS LITTLEFS
#include <LITTLEFS.h> //powaaaa, shaved down 1s in webpage load time & basically everything
//#include "SPIFFS.h"

#include <driver/adc.h>

//8x8 matrix
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 11
#define CLK_PIN 18
#define DATA_PIN 23 // or MOSI
#define CS_PIN 5

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

//web / wifi
AsyncWebServer server(80);

const char* ssid = "Furo";
const char* password = "FuroProto";

// Ears LEDs
#define NumOfEarsLEDS 74
#define DATA_PIN 16
#define DATA_PIN_BLUSH 17
CRGB leds[NumOfEarsLEDS];
CRGB rainbow_buffer[4];
CRGB blushLeds[8];

//Structs for anims in memory
struct FramesEars {
  int timespan;
  String leds[NumOfEarsLEDS];
};  

struct AnimNowEars {
  bool isCustom;
  String prefab;
  int numOfFrames;
  FramesEars frames[16];
};

struct FramesVisor {
  int timespan;
  uint64_t leds[11];
  String ledsBlush[8];
};  

struct AnimNowVisor {
  bool isCustom;
  String prefab;
  int numOfFrames;
  FramesVisor frames[16];
};

AnimNowEars earsNow;
AnimNowVisor visorNow;

struct TalkingAnimStruct {
  uint64_t mouthOpen[6];
  uint64_t mouthClosed[6];
};

TalkingAnimStruct talkingAnim;

String currentAnim = "";
int currentEarsFrame = 1, currentVisorFrame = 1;
//config vars
bool speechEna = false, boopEna = false, tiltEna = false, instantReload = false;
int bEar = 64, bVisor = 6, rbSpeed = 15, rbWidth = 8, spMin = 90, spMax = 130;
String aTilt = "confused.json", aDown = "sad.json", aUp = "rlly.json";

bool loadAnim(String anim, String temp) {
  DynamicJsonDocument doc(32768); //will be probs not enough
  
  if(anim == "POSTAnimLoad") {
    Serial.println("POST");

    DeserializationError error = deserializeJson(doc, temp);
    if (error){
      Serial.println("Failed to deserialize POST of a animation!");
      return false;
    }
  } else {
    File file = SPIFFS.open("/anims/"+anim, "r");
    if (!file) {
      Serial.println("There was an error opening the animation file!");
      file.close();
      return false;
    }
    Serial.println("Animation file opened!");

    ReadBufferingStream bufferedFile{file, 64};

    DeserializationError error = deserializeJson(doc, bufferedFile);
    if (error){
      Serial.println("Failed to deserialize animation file!");
      return false;
    }

    file.close();
  }
  
  currentAnim = anim;
      
  if(doc["ears"]["type"].as<String>() == "custom") {
    earsNow.isCustom = true;
    earsNow.numOfFrames = doc["ears"]["frames"].size();
    for(int x = 0; x < earsNow.numOfFrames; x++) {
      earsNow.frames[x].timespan = doc["ears"]["frames"][x]["timespan"].as<int>();
      int ledsCount = doc["ears"]["frames"][x]["leds"].size();
      for(int y = 0; y < ledsCount; y++) {
        earsNow.frames[x].leds[y] = doc["ears"]["frames"][x]["leds"][y].as<String>();
      }
    }
  } else if (doc["ears"]["type"].as<String>() == "prefab") {
    earsNow.isCustom = false;
    earsNow.prefab = doc["ears"]["prefab"].as<String>();
  } else {
    Serial.println("Something is wrong with ears-type!");
  }
      
  if(doc["visor"]["type"].as<String>() == "custom") {
    visorNow.isCustom = true; 
    visorNow.numOfFrames = doc["visor"]["frames"].size();
    for(int x = 0; x < visorNow.numOfFrames; x++) {
      visorNow.frames[x].timespan = doc["visor"]["frames"][x]["timespan"].as<int>();
      int ledsCount = doc["visor"]["frames"][x]["leds"].size();
      for(int y = 0; y < ledsCount; y++) {
        visorNow.frames[x].leds[y] = strtoull(String(doc["visor"]["frames"][x]["leds"][y].as<String>()).c_str(), NULL, 16); //string to uint64
      }
      int ledsBlushCount = doc["visor"]["frames"][x]["ledsBlush"].size();
      for(int y = 0; y < ledsBlushCount; y++) {
        visorNow.frames[x].ledsBlush[y] = doc["visor"]["frames"][x]["ledsBlush"][y].as<String>();
      }
    }
  } else if (doc["visor"]["type"] == "prefab") {
    visorNow.isCustom = false;
    visorNow.prefab = doc["visor"]["prefab"].as<String>();
  } else {
    Serial.println("Something is wrong with visor-type!");
  }
  instantReload = true;
  currentVisorFrame = 1;
  currentEarsFrame = 1;
  return true;
}

bool loadMouthAnim() {
  DynamicJsonDocument doc(1024);
  
  File file = SPIFFS.open("/anims/_mouth.json", "r");
  if (!file) {
    Serial.println("There was an error opening the mouth animation file!");
    file.close();
    return false;
  }
  Serial.println("Mouth animation file opened!");

  DeserializationError error = deserializeJson(doc, file);
  if (error){
    Serial.println("Failed to deserialize mouth animation file!");
    return false;
  }
  file.close();
  
  for(int i = 0;i<6;i++) {
    talkingAnim.mouthClosed[i] = strtoull(String(doc["mouthClosed"][i].as<String>()).c_str(), NULL, 16);
    talkingAnim.mouthOpen[i] = strtoull(String(doc["mouthOpened"][i].as<String>()).c_str(), NULL, 16);
  }
  return true;
}

bool loadConfig() {
  DynamicJsonDocument doc(512);
  
  File file = SPIFFS.open("/config.json", "r");
  if (!file) {
    Serial.println("There was an error opening the config file!");
    file.close();
    return false;
  }
  Serial.println("Config file opened!");

  DeserializationError error = deserializeJson(doc, file);
  if (error){
    Serial.println("Failed to deserialize the config file!");
    return false;
  }
  file.close();

  boopEna = doc["boopEna"].as<bool>();
  speechEna = doc["speechEna"].as<bool>();
  tiltEna = doc["tiltEna"].as<bool>();
  bEar = doc["bEar"].as<int>();
  bVisor = doc["bVisor"].as<int>();
  rbSpeed = doc["rbSpeed"].as<int>();
  rbWidth = doc["rbWidth"].as<int>();
  spMin = doc["spMin"].as<int>();
  spMax = doc["spMax"].as<int>();
  aTilt = doc["aTilt"].as<String>();
  aDown = doc["aDown"].as<String>();
  aUp = doc["aUp"].as<String>();

  return true;
}

bool saveConfig() {
  DynamicJsonDocument doc(512);
  
  File file = SPIFFS.open("/config.json", "w");
  if (!file) {
    Serial.println("There was an error opening the config file!");
    file.close();
    return false;
  }
  Serial.println("Config file opened!");

  doc["boopEna"] = boopEna;
  doc["speechEna"] = speechEna;
  doc["tiltEna"] = tiltEna;
  doc["bEar"] = bEar;
  doc["bVisor"] = bVisor;
  doc["rbSpeed"] = rbSpeed;
  doc["rbWidth"] = rbWidth;
  doc["spMin"] = spMin;
  doc["spMax"] = spMax;
  doc["aTilt"] = aTilt;
  doc["aDown"] = aDown;
  doc["aUp"] = aUp;

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to deserialize the config file");
    return false;
  }
  
  file.close();

  FastLED.setBrightness(bEar);
  mx.control(MD_MAX72XX::INTENSITY, bVisor);

  return true;
}

void setup() {
  Serial.begin(115200);

  pinMode(35, INPUT);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println(IP);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  server.serveStatic("/", LITTLEFS, "/").setDefaultFile("index.html");

  server.on("/getfiles", HTTP_GET, [](AsyncWebServerRequest *request){ //returns current anim + all avaible anims
    String files = currentAnim+";";
    File root = SPIFFS.open("/anims");
    File file = root.openNextFile();
    while(file){
      files += String(file.name()).substring(7) + ";";
      file = root.openNextFile();
    }
    request->send(200, "text/plain", files);
  });
  
  server.on("/saveconfig", HTTP_GET, [](AsyncWebServerRequest *request){ //saves config
    if(request->hasParam("boopEna"))
      std::istringstream(request->getParam("boopEna")->value().c_str()) >> std::boolalpha >> boopEna;
    if(request->hasParam("speechEna"))
      std::istringstream(request->getParam("speechEna")->value().c_str()) >> std::boolalpha >> speechEna;
    if(request->hasParam("tiltEna"))
      std::istringstream(request->getParam("tiltEna")->value().c_str()) >> std::boolalpha >> tiltEna;
    if(request->hasParam("bEar"))
      bEar = request->getParam("bEar")->value().toInt();
    if(request->hasParam("bVisor"))
      bVisor = request->getParam("bVisor")->value().toInt();
    if(request->hasParam("rbSpeed"))
      rbSpeed = request->getParam("rbSpeed")->value().toInt();
    if(request->hasParam("rbWidth"))
      rbWidth = request->getParam("rbWidth")->value().toInt();
    if(request->hasParam("spMin"))
      spMin = request->getParam("spMin")->value().toInt();
    if(request->hasParam("spMax"))
      spMax = request->getParam("spMax")->value().toInt();
    if(saveConfig()) {
      request->redirect("/");
    } else {
      request->send(200, "text/plain", "Saving config failed!");
    }
  });

  server.on("/savefile", HTTP_POST, [](AsyncWebServerRequest *request){ //saves data from POST to file
    if(request->hasParam("file", true) && request->hasParam("content", true)) {
      File file = SPIFFS.open("/anims/"+request->getParam("file", true)->value()+".json", "w");
      if (!file) {
        Serial.println("There was an error opening the file for saving a animation!");
        file.close();
        request->send(200, "text/plain", "Error opening file for writing!");
      } else {
        Serial.println("File saved!");
        file.print(request->getParam("content", true)->value());
        file.close();
        if(request->getParam("file",true)->value() == "_mouth") {
          if(!loadMouthAnim()) {
            request->send(200, "text/plain", "Failed to load mouth animation!");
          }
        }
        request->redirect("/animator.html");
      }
    } else {
      request->send(200, "text/plain", "No valid parameters detected!");
    }
  });

  server.on("/deletefile", HTTP_GET, [](AsyncWebServerRequest *request){ //deletes asked file
    if(request->hasParam("file")) {
      SPIFFS.remove("/anims/"+request->getParam("file")->value());
      request->redirect("/");
    } else {
      request->send(200, "text/plain", "Parameter 'file' not present!");
    }
  });

  server.on("/change", HTTP_GET, [](AsyncWebServerRequest *request){ //loads anim from selected avaible anims
    if(request->hasParam("anim")) {
      if(loadAnim(request->getParam("anim")->value(),"")) {
        request->redirect("/");
      } else {
        request->send(200, "text/plain", "Loading animation has failed!");
      }
    } else {
      request->send(200, "text/plain", "No valid parameters detected!");
    }
  });

  server.on("/change", HTTP_POST, [](AsyncWebServerRequest *request){ //loads anim from POST request
    if(request->hasParam("anim", true)) {
      if(loadAnim("POSTAnimLoad",request->getParam("anim", true)->value())) {
        request->redirect("/");
      } else {
        request->send(200, "text/plain", "Loading animation has failed!");
      }
    } else {
      request->send(200, "text/plain", "No valid parameters detected!");
    }
  });

  server.onNotFound([](AsyncWebServerRequest *request){request->send(404, "text/plain", "Not found");});
  server.begin();
  
  if(!loadConfig()) {
    Serial.println("There was a problem with loading the config!");
  }

  if(!loadMouthAnim()) {
    Serial.println("There was a problem with loading the mouth frames!");
  }

  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, bVisor);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NumOfEarsLEDS);
  FastLED.addLeds<WS2812B, DATA_PIN_BLUSH, RGB>(blushLeds, 8);
  FastLED.setCorrection(TypicalPixelString);
  FastLED.setBrightness(bEar);
  
  loadAnim("default.json","");
}

String oldanim;
bool booping = false, wasTilt = false, speechFirst = true, speechResetDone = false;
float zAx,yAx;
int speech = 0, tRead, boops = 0, randomNum, randomTimespan = 0, fixVal, matrixFix = 7, blushFix;
unsigned int lastMillsEars = 0, lastMillsVisor = 0, lastMillsTilt = 0, lastMillsSpeech = 0, lastSpeak = 0, lastMillsBoop = 0, lastMillsSpeechAnim = 0, lastGoodTilt = 0;
byte row = 0;

void loop() {
  if(earsNow.isCustom) { //EARS
    if(lastMillsEars+earsNow.frames[currentEarsFrame-1].timespan <= millis() || instantReload) {
      if(currentEarsFrame == earsNow.numOfFrames) {
        currentEarsFrame = 0;
      }
      //Serial.println("setting leds with frame n."+String(currentEarsFrame)+" after "+String(millis()-lastMillsEars));
      for(int y = 0; y < NumOfEarsLEDS; y++) {
        if(earsNow.frames[currentEarsFrame].leds[y] == "0") {
          leds[y] = 0x000000;
        } else {
          leds[y] = strtol(earsNow.frames[currentEarsFrame].leds[y].c_str(), NULL, 16);
        }
      }
      FastLED.show();
      lastMillsEars = millis();
      currentEarsFrame++;
    }
  } else if (earsNow.prefab == "rainbow") {
    fill_rainbow(rainbow_buffer, 4, millis()/rbSpeed, 255/rbWidth);
    for(int x = 0;x<NumOfEarsLEDS;x++) { //xD
      if(x<16) {
        leds[x] = rainbow_buffer[0];
        leds[x+37] = rainbow_buffer[0];
      } else if(x<28) {
        leds[x] = rainbow_buffer[1];
        leds[x+37] = rainbow_buffer[1];
      } else if(x<36) {
        leds[x] = rainbow_buffer[2];
        leds[x+37] = rainbow_buffer[2];
      } else if(x==36) {
        leds[x] = rainbow_buffer[3];
        leds[x+37] = rainbow_buffer[3];
      }
    }
    FastLED.show();
  } else {
    Serial.println(earsNow.prefab);
  }

  if(visorNow.isCustom) { //VISOR+BLUSH
    if(lastMillsVisor+visorNow.frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
      if(currentVisorFrame == visorNow.numOfFrames) {
        currentVisorFrame = 0;
      }
      //Serial.println("setting visor with frame n."+String(currentVisorFrame)+" after "+String(millis()-lastMillsVisor));
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
      for(int y = 0; y < 11; y++) {
        matrixFix = 7; //fix for wrongly oriented matrixes
        if(y > 1 && y < 9 && y != 5 && speech > 2) { //if sets mouth and we are talking, do nothing
        } else {
          for (int i = 0; i < 8; i++) {
            row = (visorNow.frames[currentVisorFrame].leds[y] >> matrixFix * 8) & 0xFF;
            for (int j = 0; j < 8; j++) {
              mx.setPoint(i, j+(y*8), bitRead(row, j));
            }
            matrixFix--;
          }
        }
      }
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
      for(int x = 0; x<8; x++) {
        if(x == 4) { //dum fix for wrongly oriented strip
          blushFix = 5;
        } else if (x == 5) {
          blushFix = 4;
        } else {
          blushFix = x;
        }
        if(visorNow.frames[currentVisorFrame].ledsBlush[x] == "0") {
          blushLeds[blushFix] = 0x000000;
        } else {
          blushLeds[blushFix] = strtol(visorNow.frames[currentVisorFrame].ledsBlush[x].c_str(), NULL, 16);
        }
      }
      FastLED.show();
      lastMillsVisor = millis();
      currentVisorFrame++;
      instantReload = false;
    }
  } else {
    Serial.println(visorNow.prefab);
  }

  if(lastMillsTilt+1000<=millis() && tiltEna) { //TILT  - calibrate
    zAx = (((float)analogRead(33) - 340)/68*9.8);
    yAx = (((float)analogRead(32) - 329.5)/68.5*9.8);
    //Serial.println("Y: " + yAx + " | Z: " + zAx);
    if((zAx > 250 || zAx < 205) && yAx > 190 && yAx < 275 && !wasTilt) { // vpravo 255/222, vlevo 200/255
      Serial.println("tilt");
      wasTilt = true;
      lastGoodTilt = millis();
      oldanim = currentAnim;
      loadAnim(aTilt,"");
    } else if (yAx > 260 && zAx > 229 && zAx < 249 && !wasTilt) { // dolu 239/265
      Serial.println("dolu");
      wasTilt = true;
      lastGoodTilt = millis();
      oldanim = currentAnim;
      loadAnim(aDown,"");
    } else if (yAx < 208 && zAx > 210 && zAx < 230 && !wasTilt) { // nahoru 220/202
      Serial.println("nahoru");
      wasTilt = true;
      lastGoodTilt = millis();
      oldanim = currentAnim;
      loadAnim(aUp,"");
    } else if (zAx > 213 && zAx < 253 && yAx > 213 && yAx < 253 && wasTilt && lastGoodTilt+500<millis()) { // center 233/233
      Serial.println("no tilt");
      wasTilt = false;
      loadAnim(oldanim,"");
    }
    lastMillsTilt = millis();
  }

  if(lastMillsSpeech+20<=millis() && speechEna) { //SPEAK - calibrate
    if(digitalRead(35) == HIGH) {
      speech++;
      lastSpeak = millis();
      if(speech == 2) {
        speechResetDone = false;
        Serial.println("speeking");
      }
    }
    if(speech >= 2 && lastSpeak+500<millis()) {
      Serial.println("NOT speeking");
      speechFirst = true;
      speech = 0;
    }
    lastMillsSpeech = millis();
  }
  if(speech >= 2 && lastMillsSpeechAnim+randomTimespan<=millis()) { //speech animation
    if(speechFirst == true){
      randomNum == 0;
      speechFirst = false;
    } else {
      randomNum = random(2);
    }
    randomTimespan = random(spMin,spMax);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    for(int y = 2; y < 9; y++) {
      if(y<5){ //dum fix
        fixVal = y-2;
      } else if (y>5) {
        fixVal = y-3;
      }
      if(y != 5) {
        matrixFix = 7;
        for (int i = 0; i < 8; i++) {
          if(randomNum == 0) {
            row = (talkingAnim.mouthOpen[fixVal] >> matrixFix * 8) & 0xFF;
          } else {
            row = (talkingAnim.mouthClosed[fixVal] >> matrixFix * 8) & 0xFF;
          }
          for (int j = 0; j < 8; j++) {
            mx.setPoint(i, j+(y*8), bitRead(row, j));
          }
          matrixFix--;
        }
      }
    }
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    lastMillsSpeechAnim = millis();
  }
  if(!speechResetDone && lastMillsSpeechAnim+800<millis()) {//reset frame after speech
    for(int y = 0; y < 11; y++) {
      matrixFix = 7; //fix for wrongly oriented matrixes
      if(y > 1 && y < 9 && y != 5) { //if sets mouth reset frame
        for (int i = 0; i < 8; i++) {
          row = (visorNow.frames[currentVisorFrame-1].leds[y] >> matrixFix * 8) & 0xFF;
          for (int j = 0; j < 8; j++) {
            mx.setPoint(i, j+(y*8), bitRead(row, j));
          }
          matrixFix--;
        }
      }
    }
    speechResetDone = true;
  }

  if(millis() > 2000 && boopEna) { //BOOP - calibrate - maybe redo to that pcb with secondary psu
    tRead = digitalRead(27);
    //Serial.println(tRead);
    if(tRead == HIGH && booping == false) {
      boops++;
      if(booping == false && boops >= 2) {
        booping = true;
        //Serial.println("BOOP");
        oldanim = currentAnim;
        loadAnim("boop.json","");
      }
      lastMillsBoop = millis();
    }
    if (lastMillsBoop+1000<=millis()) {
      boops = 0;
      if(tRead == LOW && booping == true) {
        booping = false;
        //Serial.println("unBOOP");
        loadAnim(oldanim,"");
      }
    }
  }
}
