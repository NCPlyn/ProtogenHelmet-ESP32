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

#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#define CONFIG_LITTLEFS_CACHE_SIZE 256
#define SPIFFS LittleFS
#include <LittleFS.h> //powaaaa, shaved down 1s in webpage load time & basically everything

#include <driver/adc.h>

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
    void onRead(BLECharacteristic* pCharacteristic) {
      Serial.println("OnRead");
    };
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
  bool isCustom;
  String prefab;
  int numOfFrames;
  FramesEars frames[16]; //max amount of ear frames -> 16
};

struct FramesVisor { //max cca 150bytes per frame
  int timespan;
  uint64_t leds[11];
  String ledsBlush[8];
};

struct AnimNowVisor { //max cca 2420bytes with 16 frames
  bool isCustom;
  String prefab;
  int numOfFrames;
  FramesVisor frames[16]; //max amount of visor frames -> 16
};

AnimNowEars earsNow;
AnimNowVisor visorNow;

//--------------------------------//Config vars
bool speechEna = false, boopEna = false, tiltEna = false, instantReload = false;
int bEar = 64, bVisor = 6, rbSpeed = 15, rbWidth = 8, spMin = 90, spMax = 130, currentEarsFrame = 1, currentVisorFrame = 1;
String aTilt = "confused.json", aUp = "upset.json", currentAnim = "";

//--------------------------------//Load functions
bool loadAnim(String anim, String temp) {
  DynamicJsonDocument doc(24000); //will be probs not enough

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

  String eType = doc["ears"]["type"].as<String>();
  if(eType == "custom" || eType == "custom_glow") {
    if(eType == "custom") {
      earsNow.isCustom = true;
    } else if (eType == "custom_glow") {
      earsNow.isCustom = false;
      earsNow.prefab = eType;
    }
    earsNow.numOfFrames = doc["ears"]["frames"].size();
    for(int x = 0; x < earsNow.numOfFrames; x++) {
      earsNow.frames[x].timespan = doc["ears"]["frames"][x]["timespan"].as<int>();
      int ledsCount = doc["ears"]["frames"][x]["leds"].size();
      for(int y = 0; y < ledsCount; y++) {
        earsNow.frames[x].leds[y] = doc["ears"]["frames"][x]["leds"][y].as<String>();
      }
    }
  } else if (eType == "prefab") {
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
  Serial.println("Config file opened to save!");

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

//--------------------------------//Setup
void setup() {
  Serial.begin(115200);

  pinMode(35, INPUT);
  pinMode(27, INPUT);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println(IP);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

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

  server.onNotFound([](AsyncWebServerRequest *request){request->send(404, "text/plain", "Not found");});
  server.begin();

  if(!loadConfig()) {
    Serial.println("There was a problem with loading the config!");
  }

  if(!startBLE()) {
    Serial.println("There was a problem with starting BLE!");
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
bool booping = false, wasTilt = false, speechFirst = true, speechResetDone = false;
float zAx,yAx;
int speech = 0, tRead, boops = 0, randomNum, randomTimespan = 0, fixVal, matrixFix = 7, blushFix, startIndex = 1;
unsigned int lastMillsEars = 0, lastMillsVisor = 0, lastMillsTilt = 0, lastMillsSpeech = 0, lastSpeak = 0, lastMillsBoop = 0, lastMillsSpeechAnim = 0, lastGoodTilt = 0, lastFLED = 0;
byte row = 0;

void loop() {
  //--------------------------------//EAR Leds render
  if(earsNow.isCustom) {
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
  } else if (earsNow.prefab == "rainbow") {
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
  } else if (earsNow.prefab == "white_noise") {
	  memset(noiseData, 0, NumOfEarsLEDS);
	  fill_raw_noise8(noiseData, NumOfEarsLEDS, 2, 0, 50, millis()/4);
    for(int x = 0;x<NumOfEarsLEDS;x++) {
      leds[x] = ColorFromPalette(blackWhite, noiseData[x]);
    }
    FastLED.show();
  } else if (earsNow.prefab == "corner_sabers") {
    if(lastFLED+20 < millis()) {
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
  } else if (earsNow.prefab == "custom_glow") {
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
  if(visorNow.isCustom) {
    if(lastMillsVisor+visorNow.frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
      lastMillsVisor = millis();
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

  //--------------------------------//TILT (has to be calibrated)
  if(lastMillsTilt+1000<=millis() && tiltEna) {
    zAx = (((float)analogRead(33) - 340)/68*9.8);
    yAx = (((float)analogRead(32) - 329.5)/68.5*9.8);
    //Serial.println("Y: " + yAx + " | Z: " + zAx);
    if((zAx > 250 || zAx < 205) && yAx > 190 && yAx < 275 && !wasTilt) { // vpravo 255/222, vlevo 200/255
      Serial.println("tilt");
      wasTilt = true;
      lastGoodTilt = millis();
      oldanim = currentAnim;
      loadAnim(aTilt,"");
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

  //--------------------------------//SPEECH Detection
  if(lastMillsSpeech+20<=millis() && speechEna) {
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
  //--------------------------------//SPEECH Animation
  if(speech >= 2 && lastMillsSpeechAnim+randomTimespan<=millis()) {
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
  if(!speechResetDone && lastMillsSpeechAnim+800<millis()) {
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
    tRead = digitalRead(27);
    if(tRead == HIGH && booping == false) {
      booping = true;
      //Serial.println("BOOP");
      oldanim = currentAnim;
      loadAnim("boop.json","");
      lastMillsBoop = millis();
    } else if(tRead == LOW && booping == true && lastMillsBoop+1000<millis()) {
      booping = false;
      //Serial.println("unBOOP");
      loadAnim(oldanim,"");
    }
  }
}
