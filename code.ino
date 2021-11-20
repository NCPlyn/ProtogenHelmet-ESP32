//loadAnims for boop etc.. -redo for config
//speak -test - proper anim - first frame everytime open mouth
//calibrate all - prefection them (speak detection still nope)
//prefabs
// /change post crashes esp? - maybe add buffer?
//make config have brithness and setting for choosing animations for different things
//holy shit is it buggy by SW & HW side

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
#include "SPIFFS.h"

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

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// Ears LEDs
#define NumOfEarsLEDS 74
#define DATA_PIN 16
#define DATA_PIN_BLUSH 17
CRGB leds[NumOfEarsLEDS];
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
  FramesEars frames[10];
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
  FramesVisor frames[10];
};

AnimNowEars earsNow;
AnimNowVisor visorNow;

struct TalkingAnimStruct {
  uint64_t mouthOpen[6];
  uint64_t mouthClosed[6];
};

TalkingAnimStruct talkingAnim;

String currentAnim = "";
bool speechEna = false, boopEna = false, tiltEna = false, instantReload = false;;

bool loadAnim(String anim, String temp) {
  DynamicJsonDocument doc(32768); //will be probs not enough
  
  if(anim == "POSTAnimLoad") {
    Serial.println("POST");

    DeserializationError error = deserializeJson(doc, temp);
    if (error){
      Serial.println("Failed to deserialize file");
      return false;
    }
  } else {
    File file = SPIFFS.open("/anims/"+anim, "r");
    if (!file) {
      Serial.println("There was an error opening the file");
      file.close();
      return false;
    }
    Serial.println("File opened!");

    ReadBufferingStream bufferedFile{file, 64};

    DeserializationError error = deserializeJson(doc, bufferedFile);
    if (error){
      Serial.println("Failed to deserialize file");
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
  return true;
}

void setMouthAnim() {
  for(int i = 0;i<6;i++) {
    talkingAnim.mouthClosed[i] = 1099494850560;
    talkingAnim.mouthOpen[i] = 17102903382850560;
  }
}

bool loadConfig() {
  DynamicJsonDocument doc(128);
  
  File file = SPIFFS.open("/config.json", "r");
  if (!file) {
    Serial.println("There was an error opening the file");
    file.close();
    return false;
  }
  Serial.println("File opened!");

  DeserializationError error = deserializeJson(doc, file);
  if (error){
    Serial.println("Failed to deserialize file");
    return false;
  }
  file.close();

  boopEna = doc["boopEna"].as<bool>();
  speechEna = doc["speechEna"].as<bool>();
  tiltEna = doc["tiltEna"].as<bool>();
}

bool saveConfig() {
  DynamicJsonDocument doc(128);
  
  File file = SPIFFS.open("/config.json", "w");
  if (!file) {
    Serial.println("There was an error opening the file");
    file.close();
    return false;
  }
  Serial.println("File opened!");

  doc["boopEna"] = boopEna;
  doc["speechEna"] = speechEna;
  doc["tiltEna"] = tiltEna;

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to deserialize file");
    return false;
  }
  
  file.close();
}

void setup() {
  Serial.begin(115200);

  pinMode(35, INPUT);

  setMouthAnim();
  
  mx.begin();

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println(IP);

  if (!SPIFFS.begin(true)) {
    Serial.println ("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/animator", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/animator.html", String(), false);
  });

  server.on("/jquerymin.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/jquerymin.js", String(), false);
  });

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

  server.on("/getconfig", HTTP_GET, [](AsyncWebServerRequest *request){ //returns config file
    request->send(SPIFFS, "/config.json", String(), false);
  });

  server.on("/saveconfig", HTTP_GET, [](AsyncWebServerRequest *request){ //saves config
    if(request->hasParam("boopEna") && request->hasParam("speechEna") && request->hasParam("tiltEna")) {
      std::istringstream(request->getParam("boopEna")->value().c_str()) >> std::boolalpha >> boopEna;
      std::istringstream(request->getParam("speechEna")->value().c_str()) >> std::boolalpha >> speechEna;
      std::istringstream(request->getParam("tiltEna")->value().c_str()) >> std::boolalpha >> tiltEna;
      if(saveConfig()) {
        request->redirect("/");
      } else {
        request->send(200, "text/plain", "saveConfig failed");
      }
    } else {
      request->send(200, "text/plain", "Nothing/wrong data recieved");
    }
  });

  server.on("/savefile", HTTP_POST, [](AsyncWebServerRequest *request){ //saves data from POST to file
    if(request->hasParam("file", true) && request->hasParam("content", true)) {
      File file = SPIFFS.open("/anims/"+request->getParam("file", true)->value()+".json", "w");
      if (!file) {
        Serial.println("There was an error opening the file");
        file.close();
        request->send(200, "text/plain", "error opening file");
      } else {
        Serial.println("File opened!");
        file.print(request->getParam("content", true)->value());
        file.close();
        request->redirect("/animator");
      }
    } else {
      request->send(200, "text/plain", "no params");
    }
  });

  server.on("/loadfile", HTTP_GET, [](AsyncWebServerRequest *request){ //returns asked file
    if(request->hasParam("file")) {
      request->send(SPIFFS, "/anims/"+request->getParam("file")->value()+".json", String(), false);
    } else {
      request->send(200, "text/plain", "no param 'file'");
    }
  });

  server.on("/deletefile", HTTP_GET, [](AsyncWebServerRequest *request){ //deletes asked file
    if(request->hasParam("file")) {
      SPIFFS.remove("/anims/"+request->getParam("file")->value());
      request->redirect("/");
    } else {
      request->send(200, "text/plain", "no param 'file'");
    }
  });

  server.on("/change", HTTP_GET, [](AsyncWebServerRequest *request){ //loads anim from selected avaible anims
    if(request->hasParam("anim")) {
      if(loadAnim(request->getParam("anim")->value(),"")) {
        request->redirect("/");
      } else {
        request->send(200, "text/plain", "loadAnim failed");
      }
    } else {
      request->send(200, "text/plain", "Nothing/wrong data recieved");
    }
  });

  server.on("/change", HTTP_POST, [](AsyncWebServerRequest *request){ //loads anim from POST request
    if(request->hasParam("anim", true)) {
      if(loadAnim("POSTAnimLoad",request->getParam("anim", true)->value())) {
        request->redirect("/");
      } else {
        request->send(200, "text/plain", "loadAnim failed");
      }
    } else {
      request->send(200, "text/plain", "Nothing/wrong data recieved");
    }
  });

  server.onNotFound(notFound);
  server.begin();
  
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NumOfEarsLEDS);
  FastLED.addLeds<WS2812B, DATA_PIN_BLUSH, RGB>(blushLeds, 8);
  FastLED.setBrightness(128);

  if(!loadConfig()) {
    Serial.println("There was a problem with loading config");
  }
  loadAnim("default.json","");
}

String oldanim;
bool booping = false, wasTilt = false;
float zAx,yAx;
int currentEarsFrame = 0, currentVisorFrame = 1, speech = 0, tRead, boops = 0, randomNum, randomTimespan = 0, fixVal, matrixFix = 7, blushFix;
unsigned int lastMillsEars = 0, lastMillsVisor = 0, lastMillsTilt = 0, lastMillsSpeech = 0, lastSpeak = 0, lastMillsBoop = 0, lastMillsSpeechAnim = 0;
byte row = 0;
uint8_t thishue;

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
      instantReload = false;
    }
  } else if (earsNow.prefab == "rainbow") {
    fill_rainbow(leds, NumOfEarsLEDS, (beatsin8(17, 0, 255)+beatsin8(13, 0, 255))/2, 8);
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
        if(y > 1 && y < 9 && y != 5 && speech < 2) { //if sets mouth and we are not talking
          for (int i = 0; i < 8; i++) {
            row = (visorNow.frames[currentVisorFrame].leds[y] >> matrixFix * 8) & 0xFF;
            for (int j = 0; j < 8; j++) {
              mx.setPoint(i, j+(y*8), bitRead(row, j));
            }
            matrixFix--;
          }
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
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
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
    //Serial.println("----------");
    //Serial.println(zAx);
    //Serial.println(yAx);
    if((zAx > 250 || zAx < 205) && yAx > 190 && yAx < 275 && !wasTilt) { // vpravo 255/222, vlevo 200/255
      //headtilt - confused
      Serial.println("tilt");
      wasTilt = true;
      /*oldanim = currentAnim;
      loadAnim("tilt.json","");*/
    } else if (yAx > 260 && zAx > 229 && zAx < 249 && !wasTilt) { // dolu 239/265
      //dolů - sad (shy led blue)
      Serial.println("dolu");
      wasTilt = true;
      /*oldanim = currentAnim;
      loadAnim("sad.json","");*/
    } else if (yAx < 208 && zAx > 210 && zAx < 230 && !wasTilt) { // nahoru 220/202
      //nahoru -_-
      Serial.println("nahoru");
      wasTilt = true;
      /*oldanim = currentAnim;
      loadAnim("rly.json","");*/
    } else if (zAx > 213 && zAx < 253 && yAx > 213 && yAx < 253 && wasTilt) { // center 233/233
      Serial.println("no tilt");
      wasTilt = false;
      //loadAnim(oldanim,"");
    }
    lastMillsTilt = millis();
  }

  if(lastMillsSpeech+20<=millis() && speechEna) { //SPEAK - calibrate
    if(digitalRead(35) == HIGH) {
      speech++;
      lastSpeak = millis();
      if(speech == 2) {
        Serial.println("speeking");
      }
    }
    if(speech >= 2 && lastSpeak+500<millis()) {
      Serial.println("NOT speeking");
      speech = 0;
    }
    lastMillsSpeech = millis();
  }
  if(speech >=  2 && lastMillsSpeechAnim+randomTimespan<=millis()) { //speech animation
    randomNum = random(2);
    randomTimespan = random(200-600);
    for(int y = 2; y < 10; y++) {
      if(y<5){ //dum fix
        fixVal = y-2;
      } else if (y>5) {
        fixVal = y-3;
      }
      if(y != 5) {
        for (int i = 0; i < 8; i++) {
          if(randomNum == 0) {
            row = (talkingAnim.mouthOpen[fixVal] >> i * 8) & 0xFF;
          } else {
            row = (talkingAnim.mouthClosed[fixVal] >> i * 8) & 0xFF;
          }
          for (int j = 0; j < 8; j++) {
            mx.setPoint(i, j+(y*8), bitRead(row, j));
          }
        }
      }
    }
  }

  if(millis() > 2000 && boopEna) { //BOOP - calibrate
    tRead = touchRead(27);
    //Serial.println(tRead);
    if(tRead < 50 && booping == false) {
      boops++;
      if(booping == false && boops >= 3) {
        booping = true;
        Serial.println("BOOP");
        /*oldanim = currentAnim;
        loadAnim("boop.json","");*/
      }
      lastMillsBoop = millis();
    }
    if (lastMillsBoop+1000<=millis()) {
      boops = 0;
      if(tRead > 52 && booping == true) {
        booping = false;
        Serial.println("unBOOP");
        //loadAnim(oldanim,"");
      }
    }
  }
}
