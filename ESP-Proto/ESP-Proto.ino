// MATRIX: 5 - CS, 18 - CLK, 23 - MOSI (1st: left segment of right eye, 2st: right segment, 3th: left segment of right cheek...... 6th nose, 7th left segment of left cheek,.... 10st left segment of left eye....)
// WS2812: 16 - Ears (from outer to inner, right cheek first), 17 - Blush (from top to bottom, right cheek nearest to ear first) 
// Microphone: 35
// Touch sensor: 27
// Gyro: 22 - SCL, 21 - SDA

#include <StreamUtils.h>
#include <sstream>
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include <ArduinoJson.h>

#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#define CONFIG_LITTLEFS_CACHE_SIZE 256
#define SPIFFS LittleFS
#include <LittleFS.h>

#include "SparkFunLSM6DS3.h" 
#include "Wire.h"
LSM6DS3 myIMU;

bool loadAnim(String anim, String temp);

//--------------------------------//web / wifi
#include "WiFi.h"
#include "ESPAsyncWebServer.h"

AsyncWebServer server(80);

const char* ssid = "Proto";
const char* password = "Proto123";

//--------------------------------//BLE
#include "NimBLEDevice.h"

//add to config
String BLEfiles[20];
uint8_t BLEnum;

BLEServer *pServer = NULL;
BLECharacteristic * pCharacteristic;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
      String temp = String(pCharacteristic->getValue().c_str());
      if(temp.charAt(0) == 'g') {
        pCharacteristic->setValue("i"+String(BLEnum));
        pCharacteristic->notify(true);
      } else if (temp.toInt() > 0 && temp.toInt() <= BLEnum){
        loadAnim(BLEfiles[temp.toInt()-1],"");
      }
      Serial.println(temp);
    };
};

bool startBLE() {
  File root = SPIFFS.open("/anims");
  File file = root.openNextFile();
  BLEnum = 0;
  while(file){
    String tempName = String(file.name());
    if(tempName.charAt(0) != '_') {
      BLEfiles[BLEnum] = tempName;
      BLEnum++;
    }
    file = root.openNextFile();
  }

  BLEDevice::init("ProtoBLE");

  pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService("ffe0");

  pCharacteristic = pService->createCharacteristic("ffe1",
      NIMBLE_PROPERTY::BROADCAST | NIMBLE_PROPERTY::READ  |
      NIMBLE_PROPERTY::NOTIFY    | NIMBLE_PROPERTY::WRITE |
      NIMBLE_PROPERTY::INDICATE
  );

  pCharacteristic->setValue(BLEnum);
  pCharacteristic->setCallbacks(new MyCallbacks());

  if(!pService->start()) {
    return false;
  }

  BLEAdvertising* pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(BLEUUID(pService->getUUID()));
  if(!pAdvertising->start(0)) {
    return false;
  }

  return true;
}

//--------------------------------//Light segments
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 11
#define CLK_PIN 18
#define DATA_PIN 23 // or MOSI
#define CS_PIN 5

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

//--------------------------------//Ears LEDs
#define NumOfEarsLEDS 74
#define DATA_PIN 16
#define DATA_PIN_BLUSH 17
CRGB leds[NumOfEarsLEDS];
CRGB pixelBuffer[18];
CRGB blushLeds[8];
uint8_t noiseData[NumOfEarsLEDS];

DEFINE_GRADIENT_PALETTE( blackWhite_gp ) {
  0,   100,  0, 0,
  120,   0,  0, 0,
  255, 255,  255, 255
};
CRGBPalette16 blackWhite = blackWhite_gp;

const std::vector<std::vector<int>> lookupDiag1 = 
{{14,15,16},
 {13,27,28,1},
 {12,26,36,17,2},
 {25,35,29,18},
 {11,34,37,30,3},
 {24,33,31,19},
 {10,23,32,20,4},
 {9,22,21,5},
 {8,7,6}};

const std::vector<std::vector<int>> lookupDiag2 = 
{{2,3,4},
 {1,18,19,5},
 {16,17,30,20,6},
 {28,29,31,21},
 {15,36,37,32,7},
 {27,35,33,22},
 {14,26,34,23,8},
 {13,25,24,9},
 {12,11,10}};

//--------------------------------//Structs for anims in memory (+-11kb of heap)
struct FramesEars { //max cca 520bytes per frame
  int timespan;
  String leds[NumOfEarsLEDS];
};

struct AnimNowEars { //max cca 8330bytes with 16 frames
  String type;
  int numOfFrames;
  FramesEars frames[16]; //max amount of ear frames -> 16
};

struct FramesVisor { //max cca 150bytes per frame
  int timespan;
  uint64_t leds[11];
  String ledsBlush[8];
};

struct AnimNowVisor { //max cca 2420bytes with 16 frames
  String type;
  int numOfFrames;
  FramesVisor frames[16]; //max amount of visor frames -> 16
};

AnimNowEars earsNow;
AnimNowVisor visorNow;

//--------------------------------//Config vars
bool speechEna = false, boopEna = false, tiltEna = false, bleEna = false, instantReload = false, caliAccy = false;
int bEar = 64, bVisor = 6, rbSpeed = 15, rbWidth = 8, spMin = 90, spMax = 130, spTrig = 1500, currentEarsFrame = 0, currentVisorFrame = 0;
String aTilt = "confused.json", aUp = "upset.json", currentAnim = "";
float tiltAccy = 0.8, upAccy = -0.8, neutralX = 0, neutralZ = 0;

//--------------------------------//Load functions
bool loadAnim(String anim, String temp) {
  DynamicJsonDocument doc(24000); //will be probs not enough

  if(anim == "POSTAnimLoad") {
    Serial.println("POST");
    if(deserializeJson(doc, temp)){
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

    if(deserializeJson(doc, bufferedFile)){
      Serial.println("Failed to deserialize animation file!");
      return false;
    }

    file.close();
  }

  currentAnim = anim;

  earsNow.type = doc["ears"]["type"].as<String>();
  earsNow.numOfFrames = doc["ears"]["frames"].size();
  for(int x = 0; x < earsNow.numOfFrames; x++) {
    earsNow.frames[x].timespan = doc["ears"]["frames"][x]["timespan"].as<int>();
    for(int y = 0; y < doc["ears"]["frames"][x]["leds"].size(); y++) {
      earsNow.frames[x].leds[y] = doc["ears"]["frames"][x]["leds"][y].as<String>();
    }
  }

  visorNow.type = doc["visor"]["type"].as<String>();
  visorNow.numOfFrames = doc["visor"]["frames"].size();
  for(int x = 0; x < visorNow.numOfFrames; x++) {
    visorNow.frames[x].timespan = doc["visor"]["frames"][x]["timespan"].as<int>();
    for(int y = 0; y < doc["visor"]["frames"][x]["leds"].size(); y++) {
      visorNow.frames[x].leds[y] = strtoull(String(doc["visor"]["frames"][x]["leds"][y].as<String>()).c_str(), NULL, 16); //string to uint64
    }
    for(int y = 0; y < doc["visor"]["frames"][x]["ledsBlush"].size(); y++) {
      visorNow.frames[x].ledsBlush[y] = doc["visor"]["frames"][x]["ledsBlush"][y].as<String>();
    }
  }
  instantReload = true;
  currentVisorFrame = 0;
  currentEarsFrame = 0;
  
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

  if(deserializeJson(doc, file)){
    Serial.println("Failed to deserialize the config file!");
    return false;
  }
  file.close();

  boopEna = doc["boopEna"].as<bool>();
  speechEna = doc["speechEna"].as<bool>();
  tiltEna = doc["tiltEna"].as<bool>();
  bleEna = doc["bleEna"].as<bool>();
  bEar = doc["bEar"].as<int>();
  bVisor = doc["bVisor"].as<int>();
  rbSpeed = doc["rbSpeed"].as<int>();
  rbWidth = doc["rbWidth"].as<int>();
  spMin = doc["spMin"].as<int>();
  spMax = doc["spMax"].as<int>();
  spTrig = doc["spTrig"].as<int>();
  aTilt = doc["aTilt"].as<String>();
  aUp = doc["aUp"].as<String>();
  tiltAccy = doc["tiltAccy"].as<float>();
  upAccy = doc["upAccy"].as<float>();
  neutralX = doc["neutralX"].as<float>();
  neutralZ = doc["neutralZ"].as<float>();

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
  Serial.println("Config file opened to save!");

  doc["boopEna"] = boopEna;
  doc["speechEna"] = speechEna;
  doc["tiltEna"] = tiltEna;
  doc["bleEna"] = bleEna;
  doc["bEar"] = bEar;
  doc["bVisor"] = bVisor;
  doc["rbSpeed"] = rbSpeed;
  doc["rbWidth"] = rbWidth;
  doc["spMin"] = spMin;
  doc["spMax"] = spMax;
  doc["spTrig"] = spTrig;
  doc["aTilt"] = aTilt;
  doc["aUp"] = aUp;
  doc["tiltAccy"] = tiltAccy;
  doc["upAccy"] = upAccy;
  doc["neutralX"] = neutralX;
  doc["neutralZ"] = neutralZ;

  if(serializeJson(doc, file) == 0) {
    Serial.println("Failed to deserialize the config file");
    return false;
  }

  file.close();

  FastLED.setBrightness(bEar);
  mx.control(MD_MAX72XX::INTENSITY, bVisor);

  return true;
}

//--------------------------------//Dynamic speak anim
uint64_t speakMatrix(uint64_t input) {
  int darray[8][8];
  for (int i = 0; i < 8; i++) { //convert int64 to 2darray
    byte row = (input >> i * 8) & 0xFF;
    for (int j = 0; j < 8; j++) {
      darray[i][j] = bitRead(row, j);
    }
  }
  for (int i = 0; i < 8; i++) { //do something to 2darray
    for (int j = 0; j < 8; j++) {
      if(darray[j][i] == 1 && j != 0) {
        darray[j-1][i] = 1;
        break;
      }
    }
    for (int j = 7; j > -1; j--) {
      if(darray[j][i] == 1 && j != 7) {
        darray[j+1][i] = 1;
        break;
      }
    }
  }
  uint64_t out = 0;
  for (int i = 0; i < 8; i++) { //convert 2darray to int64
    byte row = 0;
    for (int j = 0; j < 8; j++) {
      bitWrite(row, j, darray[i][j]);
    }
    out |= (uint64_t)row << i * 8;
  }
  return out;
}

//--------------------------------//Accel check if neutral
bool isNeutral() {
  if(myIMU.readFloatAccelX() < neutralX+0.2 && myIMU.readFloatAccelX() > neutralX-0.2 && myIMU.readFloatAccelZ() < neutralZ+0.2 && myIMU.readFloatAccelZ() > neutralZ-0.2) {
    return true;
  } else {
    return false;
  }
}

//--------------------------------//WiFi server setup
void startWiFiWeb() {
  WiFi.softAP(ssid, password);

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.on("/getfiles", HTTP_GET, [](AsyncWebServerRequest *request){ //returns current anim + all avaible anims
    String files = currentAnim+";";
    File root = SPIFFS.open("/anims");
    File file = root.openNextFile();
    while(file){
      files += String(file.name()) + ";";
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
    if(request->hasParam("bleEna"))
      std::istringstream(request->getParam("bleEna")->value().c_str()) >> std::boolalpha >> bleEna;
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
    if(request->hasParam("spTrig"))
      spTrig = request->getParam("spTrig")->value().toInt();
    if(request->hasParam("aTilt"))
      aTilt = String(request->getParam("aTilt")->value());
    if(request->hasParam("aUp"))
      aUp = String(request->getParam("aUp")->value());
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

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *request){
    caliAccy = true;
    request->send(200);
  });

  server.onNotFound([](AsyncWebServerRequest *request){request->send(404, "text/plain", "Not found");});
  server.begin();
}

//--------------------------------//Setup
void setup() {
  Serial.begin(115200);

  pinMode(35, INPUT);
  pinMode(27, INPUT);
  pinMode(13, INPUT_PULLUP);

  if(!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS!");
  }

  if(!loadConfig()) {
    Serial.println("An Error has occurred while loading config file!");
  }

  if(myIMU.begin()) {
    Serial.println("An Error has occurred while connecting to LSM!");
    tiltEna = false;
  }

  if(digitalRead(13) == HIGH) { //Pulling pin 13 LOW disables WiFi
    startWiFiWeb();
  }

  if(bleEna) { //you can disable BLE in config
    if(!startBLE()) {
      Serial.println("An Error has occurred while starting BLE!");
    }
  }
  
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, bVisor);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NumOfEarsLEDS);
  FastLED.addLeds<WS2812B, DATA_PIN_BLUSH, RGB>(blushLeds, 8);
  FastLED.setCorrection(TypicalPixelString);
  FastLED.setBrightness(bEar);

  loadAnim("default.json","");

  Serial.println("Free memory: " + String(esp_get_free_heap_size()) + " bytes");
}

//--------------------------------//Loop vars
String oldanim;
bool booping = false, wasTilt = false, speechFirst = true, speechResetDone = false, speak = false, boopRea = false;
float zAx,yAx,finalMicAvg,nvol,micline,avgMicArr[10];
int randomNum,boopRead, randomTimespan = 0, matrixFix = 7, startIndex = 1, speaking = 0, currentMicAvg = 0;
unsigned int lastMillsEars = 0, lastMillsVisor = 0, lastMillsTilt = 0, laskSpeakCheck = 0, lastSpeak = 0, lastMillsBoop = 0, lastMillsSpeechAnim = 0, lastFLED = 0;
byte row = 0;

void loop() {
  //--------------------------------//EAR Leds render
  if(earsNow.type == "custom") {
    if(lastMillsEars+earsNow.frames[currentEarsFrame-1].timespan <= millis() || instantReload) {
      lastMillsEars = millis();
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
      currentEarsFrame++;
      FastLED.show();
    }
  } else if (earsNow.type == "rainbow") {
    fill_rainbow(pixelBuffer, 4, millis()/rbSpeed, 255/rbWidth);
    for(int x = 0;x<NumOfEarsLEDS;x++) {
      if(x<16) {
        leds[x] = pixelBuffer[0];
        leds[x+37] = pixelBuffer[0];
      } else if(x<28) {
        leds[x] = pixelBuffer[1];
        leds[x+37] = pixelBuffer[1];
      } else if(x<36) {
        leds[x] = pixelBuffer[2];
        leds[x+37] = pixelBuffer[2];
      } else if(x==36) {
        leds[x] = pixelBuffer[3];
        leds[x+37] = pixelBuffer[3];
      }
    }
    FastLED.show();
  } else if (earsNow.type == "white_noise") {
	  memset(noiseData, 0, NumOfEarsLEDS);
	  fill_raw_noise8(noiseData, NumOfEarsLEDS, 2, 0, 50, millis()/4);
    for(int x = 0;x<NumOfEarsLEDS;x++) {
      leds[x] = ColorFromPalette(blackWhite, noiseData[x]);
    }
    FastLED.show();
  } else if (earsNow.type == "corner_sabers") {
    if(lastFLED+rbSpeed < millis()) {
      lastFLED = millis();
      startIndex++;
      int tempIndex = startIndex;
      for(int x = 0;x<18;x++) {
        pixelBuffer[x] = ColorFromPalette(RainbowStripeColors_p, tempIndex, 255, NOBLEND);
        tempIndex+=3;
      }
      for(int x = 0;x<9;x++) {
        for(int y = 0;y<lookupDiag1[x].size();y++) {
          leds[lookupDiag2[x][y]-1] = pixelBuffer[x];
          leds[lookupDiag1[x][y]+36] = pixelBuffer[x];
        }
      }
      FastLED.show();
    }
  } else if (earsNow.type == "custom_glow") {
    fill_rainbow(pixelBuffer, 4, millis()/rbSpeed, 255/rbWidth);
    for(int y = 0; y < NumOfEarsLEDS; y++) {
      if(earsNow.frames[0].leds[y] == "0") {
        leds[y] = 0x000000;
      } else {
        leds[y] = pixelBuffer[0];
      }
    }
    FastLED.show();
  } else {
    //....
  }

  //--------------------------------//VISOR+BLUSH Leds render
  if(visorNow.type == "custom") {
    if(lastMillsVisor+visorNow.frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
      lastMillsVisor = millis();
      if(currentVisorFrame == visorNow.numOfFrames) {
        currentVisorFrame = 0;
      }
      //Serial.println("setting visor with frame n."+String(currentVisorFrame)+" after "+String(millis()-lastMillsVisor));
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
      for(int y = 0; y < 11; y++) {
        matrixFix = 7; //fix for wrongly oriented matrixes
        if(y > 1 && y < 9 && y != 5 && speak) { //if sets mouth and we are talking, do nothing
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
        if(visorNow.frames[currentVisorFrame].ledsBlush[x] == "0") {
          blushLeds[x] = 0x000000;
        } else {
          blushLeds[x] = strtol(visorNow.frames[currentVisorFrame].ledsBlush[x].c_str(), NULL, 16);
        }
      }
      FastLED.show();
      currentVisorFrame++;
      instantReload = false;
    }
  } else {
    //....
  }

  //--------------------------------//TILT Calibration
  if(caliAccy) {
    bool upSet = false, tiltSet = false;
    neutralX = floor(myIMU.readFloatAccelX() * 100) / 100;
    neutralZ = floor(myIMU.readFloatAccelZ() * 100) / 100;
    upAccy = 0;
    tiltAccy = 0;
    do {
      if(upAccy > 0.3 && isNeutral())
        upSet = true;
      if(tiltAccy > 0.3 && isNeutral())
        tiltSet = true;
      upAccy = max(upAccy, floor(abs(myIMU.readFloatAccelX()-neutralX) * 100) / 100);
      tiltAccy = max(tiltAccy, floor(abs(myIMU.readFloatAccelZ()-neutralZ) * 100) / 100);
    } while (!upSet || !tiltSet);
    caliAccy = false;
    saveConfig();
  }

  //--------------------------------//TILT
  if(lastMillsTilt+500<=millis() && tiltEna) {
    //Serial.println(myIMU.readFloatAccelX());
    //Serial.println(myIMU.readFloatAccelZ());
    if(abs(myIMU.readFloatAccelX()-neutralX) > upAccy && !wasTilt) {
      Serial.println("up");
      wasTilt = true;
      oldanim = currentAnim;
      loadAnim(aUp,"");
    } else if (abs(myIMU.readFloatAccelZ()-neutralX) > tiltAccy && !wasTilt) {
      Serial.println("tilt");
      wasTilt = true;
      oldanim = currentAnim;
      loadAnim(aTilt,"");
    } else if (isNeutral() && wasTilt) {
      Serial.println("neutral");
      wasTilt = false;
      loadAnim(oldanim,"");
    }
    lastMillsTilt = millis();
  }

  //--------------------------------//SPEECH Detection
  if(speechEna) {
    finalMicAvg = 0;
    nvol = 0;
    for (int i = 0; i<128; i++){
      micline = abs(analogRead(35) - 512);
      nvol = max(micline, nvol);
    }
    avgMicArr[currentMicAvg] = nvol;
    if(currentMicAvg == 9) {
      currentMicAvg = 0;
    } else {
      currentMicAvg++;
    }
    for (int i = 0; i<10; i++){
      finalMicAvg+=avgMicArr[i];
    }
    //Serial.println(finalMicAvg/10);
  }
  if(speechEna && laskSpeakCheck+10<=millis()) {
    if(finalMicAvg/10 > spTrig) {
      speaking++;
      lastSpeak = millis();
    }
    if(speaking > 4 && !speak) {
      speak = true;
      speechResetDone = false;
      Serial.println("Speak");
    }
    if(lastSpeak+500<millis()) {
      if(speak) {
        speak = false;
        speechFirst = true;
        Serial.println("unSpeak");
      }
      speaking = 0;
    }
    laskSpeakCheck = millis();
  }
  //--------------------------------//SPEECH Animation
  if(speechEna && speak && lastMillsSpeechAnim+randomTimespan<=millis()) {
    randomTimespan = random(spMin,spMax);
    if(speechFirst == true){
      randomNum = 0;
      speechFirst = false;
    } else {
      randomNum = random(2);
    }
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    for(int y = 2; y < 9; y++) {
      if(y != 5) { //for only mouth segments
        matrixFix = 7;
        uint64_t tempSegment;
        if(randomNum == 0) {
          tempSegment = speakMatrix(visorNow.frames[currentVisorFrame-1].leds[y]);
        } else {
          tempSegment = visorNow.frames[currentVisorFrame-1].leds[y];
        }
        for (int i = 0; i < 8; i++) {
          row = (tempSegment >> matrixFix * 8) & 0xFF;
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
  //--------------------------------//SPEECH Reset frames
  if(!speechResetDone && !speak && lastMillsSpeechAnim+800<millis()) {
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

  //--------------------------------//BOOP Detection
  if(millis() > 2000 && boopEna) {
    boopRead = digitalRead(27);
    if(boopRead == HIGH && booping == false) {
      booping = true;
      Serial.println("BOOP");
      oldanim = currentAnim;
      loadAnim("boop.json","");
      lastMillsBoop = millis();
    } else if(boopRead == LOW && booping == true && lastMillsBoop+1000<millis()) {
      booping = false;
      Serial.println("unBOOP");
      loadAnim(oldanim,"");
    }
  }
}
