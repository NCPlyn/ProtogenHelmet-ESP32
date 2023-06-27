// WS2812: 5 - Face (right cheek, left segment of eye first), 16 - Ears (from outer to inner, right cheek first), 17 - Blush (from top to bottom, right cheek nearest to ear first) 
// Microphone: 35
// Touch sensor: 27
// Gyro,OLED,PowerMonitor: 22 - SCL, 21 - SDA

//ESP32(Vin), all LEDs, DAC and fan is wired with 5V
//Gyro,OLED,Microphone,PM and touch is wired with 3.3V (gyro and mic needs RC filter)

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
#include <Wire.h>
LSM6DS3 myIMU;

#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0,/* reset=*/ U8X8_PIN_NONE);

#include <Adafruit_INA219.h> //edited library in this sketch (replace 0.1R with 0.03R resistor on the board)
Adafruit_INA219 ina219;

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

//--------------------------------//Ears LEDs
#define EarLedsNum 74
#define DATA_PIN_EARS 16
#define DATA_PIN_BLUSH 17
#define VisorLedsNum 704
#define DATA_PIN_VISOR 5
CRGB earLeds[EarLedsNum];
CRGB blushLeds[8];
CRGB visorLeds[VisorLedsNum];

CLEDController *ledController[3];

CRGB pixelBuffer[18];
CRGB visorPixelBuffer[10];
uint8_t noiseData[EarLedsNum];

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
  String leds[EarLedsNum];
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
bool speechEna = false, boopEna = false, tiltEna = false, bleEna = false, oledEna = false, instantReload = false, caliAccy = false;
int rbSpeed = 15, rbWidth = 8, spMin = 90, spMax = 130, spTrig = 1500, currentEarsFrame = 0, currentVisorFrame = 0;
uint8_t bEar = 64, bVisor = 6;
String aTilt = "confused.json", aUp = "upset.json", currentAnim = "", visColorStr = "";
float tiltAccy = 0.8, upAccy = -0.8, neutralX = 0, neutralZ = 0;
unsigned long visColor = 0xFF0000;

//--------------------------------//Load functions
bool loadAnim(String anim, String temp) {
  DynamicJsonDocument doc(24000);

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

  if(oledEna) {
    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 0, 128, 17);
    u8g2.setDrawColor(1);
    displayCenter(anim.substring(0,anim.length()-5),14);
    u8g2.updateDisplayArea(0,0,16,2);
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

  if(deserializeJson(doc, file)){
    Serial.println("Failed to deserialize the config file!");
    return false;
  }
  file.close();

  boopEna = doc["boopEna"].as<bool>();
  speechEna = doc["speechEna"].as<bool>();
  tiltEna = doc["tiltEna"].as<bool>();
  bleEna = doc["bleEna"].as<bool>();
  oledEna = doc["oledEna"].as<bool>();
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
  visColorStr = doc["visColor"].as<String>();
  visColor = strtol(visColorStr.c_str()+1, NULL, 16);

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
  doc["oledEna"] = oledEna;
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
  doc["visColor"] = visColorStr;

  if(serializeJson(doc, file) == 0) {
    Serial.println("Failed to deserialize the config file");
    return false;
  }

  file.close();

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

//--------------------------------//Print centered text to oled buffer
void displayCenter(String text, uint16_t h) {
  float width = u8g2.getStrWidth(text.c_str());
  u8g2.drawStr((128 - width) / 2, h, text.c_str());
}

//--------------------------------//Float mapping
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
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
    if(request->hasParam("oledEna"))
      std::istringstream(request->getParam("oledEna")->value().c_str()) >> std::boolalpha >> oledEna;
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
    if(request->hasParam("visColor"))
      visColorStr = String(request->getParam("visColor")->value());
      visColor = strtol(visColorStr.c_str()+1, NULL, 16);
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

  if(oledEna) {
    Wire.beginTransmission(0x3c); //check for oled on address 0x3c
    if (Wire.endTransmission() == 0){
      u8g2.setBusClock(1500000);
      u8g2.begin();
      u8g2.setFlipMode(2);
      u8g2.setFont(u8g2_font_t0_22b_tf);
      u8g2.drawFrame(14, 36, 100, 9);
      if (!ina219.begin()) {
        Serial.println("An Error has occurred while finding INA219 chip!");
      } else {
        ina219.setCalibration_16V_8A();
      }
    } else {
      Serial.println("An Error has occurred while initializing SSD1306.");
      oledEna = false;
    }
  }

  ledController[0] = &FastLED.addLeds<WS2812B, DATA_PIN_EARS, GRB>(earLeds, EarLedsNum);
  ledController[1] = &FastLED.addLeds<WS2812B, DATA_PIN_BLUSH, RGB>(blushLeds, 8);
  ledController[2] = &FastLED.addLeds<WS2812B, DATA_PIN_VISOR, GRB>(visorLeds, VisorLedsNum);
  FastLED.setCorrection(TypicalPixelString);
  FastLED.setDither(0);

  loadAnim("default.json","");

  Serial.println("Free memory: " + String(esp_get_free_heap_size()) + " bytes");
}

//--------------------------------//Loop vars
String oldanim;
bool booping = false, wasTilt = false, speechFirst = true, speechResetDone = false, speak = false, boopRea = false, displayLeds = false;
float zAx,yAx,finalMicAvg,nvol,micline,avgMicArr[10];
int randomNum, boopRead, randomTimespan = 0, matrixFix = 7, startIndex = 1, speaking = 0, currentMicAvg = 0;
unsigned long lastMillsEars = 0, lastMillsVisor = 0, lastMillsTilt = 0, laskSpeakCheck = 0, lastSpeak = 0, lastMillsBoop = 0, lastMillsSpeechAnim = 0, lastFLED = 0, vaStatLast = 0;
byte row = 0;

//--------------------------------//Visor bufferer
void setAllVisor(unsigned long ledColor, int visorFrame) {
  uint64_t tempSegment;
  for(int y = 0; y < 11; y++) {
    tempSegment = visorNow.frames[visorFrame].leds[y];
    if(y > 1 && y < 9 && y != 5 && speak && randomNum == 0) { //if sets mouth and we are talking
      tempSegment = speakMatrix(visorNow.frames[visorFrame].leds[y]);
    }
    for (int i = 0; i < 8; i++) {
      row = (tempSegment >> i * 8) & 0xFF;
      matrixFix = 7;
      for (int j = 0; j < 8; j++) {
        visorLeds[(y*64)+(i*8)+((i%2!=0)?j:matrixFix)] = (bitRead(row,j))?ledColor:CRGB::Black;
        matrixFix--;
      }
    }
  }
  fadeToBlackBy(visorLeds, VisorLedsNum, 255-bVisor);
  displayLeds = true;
}

void loop() {
  //--------------------------------//EAR Leds render
  if(earsNow.type == "custom") {
    if(lastMillsEars+earsNow.frames[currentEarsFrame-1].timespan <= millis() || instantReload) {
      lastMillsEars = millis();
      if(currentEarsFrame == earsNow.numOfFrames) {
        currentEarsFrame = 0;
      }
      //Serial.println("setting leds with frame n."+String(currentEarsFrame)+" after "+String(millis()-lastMillsEars));
      for(int y = 0; y < EarLedsNum; y++) {
        if(earsNow.frames[currentEarsFrame].leds[y] == "0") {
          earLeds[y] = 0x000000;
        } else {
          earLeds[y] = strtol(earsNow.frames[currentEarsFrame].leds[y].c_str(), NULL, 16);
        }
      }
      currentEarsFrame++;
      fadeToBlackBy(earLeds, EarLedsNum, 255-bEar);
      displayLeds = true;
    }
  } else if (earsNow.type == "rainbow") {
    fill_rainbow(pixelBuffer, 4, millis()/rbSpeed, 255/rbWidth);
    for(int x = 0;x<EarLedsNum;x++) {
      if(x<16) {
        earLeds[x] = pixelBuffer[0];
        earLeds[x+37] = pixelBuffer[0];
      } else if(x<28) {
        earLeds[x] = pixelBuffer[1];
        earLeds[x+37] = pixelBuffer[1];
      } else if(x<36) {
        earLeds[x] = pixelBuffer[2];
        earLeds[x+37] = pixelBuffer[2];
      } else if(x==36) {
        earLeds[x] = pixelBuffer[3];
        earLeds[x+37] = pixelBuffer[3];
      }
    }
    fadeToBlackBy(earLeds, EarLedsNum, 255-bEar);
    displayLeds = true;
  } else if (earsNow.type == "white_noise") {
	  memset(noiseData, 0, EarLedsNum);
	  fill_raw_noise8(noiseData, EarLedsNum, 2, 0, 50, millis()/4);
    for(int x = 0;x<EarLedsNum;x++) {
      earLeds[x] = ColorFromPalette(blackWhite, noiseData[x]);
    }
    fadeToBlackBy(earLeds, EarLedsNum, 255-bEar);
    displayLeds = true;
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
          earLeds[lookupDiag2[x][y]-1] = pixelBuffer[x];
          earLeds[lookupDiag1[x][y]+36] = pixelBuffer[x];
        }
      }
      fadeToBlackBy(earLeds, EarLedsNum, 255-bEar);
      displayLeds = true;
    }
  } else if (earsNow.type == "custom_glow") {
    fill_rainbow(pixelBuffer, 4, millis()/rbSpeed, 255/rbWidth);
    for(int y = 0; y < EarLedsNum; y++) {
      if(earsNow.frames[0].leds[y] == "0") {
        earLeds[y] = 0x000000;
      } else {
        earLeds[y] = pixelBuffer[0];
      }
    }
    fadeToBlackBy(earLeds, EarLedsNum, 255-bEar);
    displayLeds = true;
  }

  //--------------------------------//VISOR+BLUSH Leds render
  if(visorNow.type == "custom") {
    if(lastMillsVisor+visorNow.frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
      lastMillsVisor = millis();
      if(currentVisorFrame == visorNow.numOfFrames) {
        currentVisorFrame = 0;
      }
      setAllVisor(visColor,currentVisorFrame);
      for(int x = 0; x<8; x++) {
        if(visorNow.frames[currentVisorFrame].ledsBlush[x] == "0") {
          blushLeds[x] = 0x000000;
        } else {
          blushLeds[x] = strtol(visorNow.frames[currentVisorFrame].ledsBlush[x].c_str(), NULL, 16);
        }
      }
      fadeToBlackBy(blushLeds, 8, 128-bVisor);
      currentVisorFrame++;
      instantReload = false;
    }
  } else if (visorNow.type == "all_rainbow") {
    if(lastMillsVisor+visorNow.frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
      lastMillsVisor = millis();
      if(currentVisorFrame == visorNow.numOfFrames) {
        currentVisorFrame = 0;
      }
    }
    fill_rainbow(visorPixelBuffer, 1, millis()/rbSpeed, 128/rbWidth);
    setAllVisor(((long)visorPixelBuffer[0].r << 16) | ((long)visorPixelBuffer[0].g << 8 ) | (long)visorPixelBuffer[0].b,currentVisorFrame);
    for(int x = 0; x<8; x++) {
      if(visorNow.frames[currentVisorFrame].ledsBlush[x] == "0") {
        blushLeds[x] = 0x000000;
      } else {
        blushLeds[x] = strtol(visorNow.frames[currentVisorFrame].ledsBlush[x].c_str(), NULL, 16);
      }
    }
    fadeToBlackBy(blushLeds, 8, 128-bVisor);
    if(lastMillsVisor+visorNow.frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
      currentVisorFrame++;
      instantReload = false;
    }
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
    for (int i = 0; i<64; i++){
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
      if(oledEna) {
        u8g2.drawStr(8, 62, "speak");
        u8g2.updateDisplayArea(0,6,8,2);
      }
      Serial.println("Speak");
    }
    if(lastSpeak+500<millis()) {
      if(speak) {
        speak = false;
        speechFirst = true;
        if(oledEna) {
          u8g2.setDrawColor(0);
          u8g2.drawBox(8, 48, 56, 16);
          u8g2.updateDisplayArea(0,6,8,2);
          u8g2.setDrawColor(1);
        }
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
    if(visorNow.type == "custom") {
      setAllVisor(visColor,currentVisorFrame-1);
    }
    
    lastMillsSpeechAnim = millis();
  }
  //--------------------------------//SPEECH Reset frames
  if(!speechResetDone && !speak && lastMillsSpeechAnim+800<millis()) {
    if(visorNow.type == "custom") {
      setAllVisor(visColor,currentVisorFrame-1);
    } else if (visorNow.type == "all_rainbow") {
      setAllVisor(((long)visorPixelBuffer[0].r << 16) | ((long)visorPixelBuffer[0].g << 8 ) | (long)visorPixelBuffer[0].b,currentVisorFrame-1);
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

  //--------------------------------//OLED routine
  if(oledEna && vaStatLast+500<millis()) {
    float BusV = ina219.getBusVoltage_V();
    int barStatus = mapfloat(BusV,5.5,8.42,0,100);
    if(barStatus > 100) {
      barStatus = 100;
    } else if (barStatus < 0) {
      barStatus = 0;
    }
    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 14, 128, 17);
    u8g2.setDrawColor(1);
    displayCenter(String(BusV)+"V "+String(ina219.getCurrent_mA()/1000)+"A",31);
    u8g2.updateDisplayArea(0,2,16,2);
    u8g2.setDrawColor(0);
    u8g2.drawBox(15, 37, 98, 7);
    u8g2.setDrawColor(1);
    u8g2.drawBox(14, 36, barStatus, 9);
    u8g2.updateDisplayArea(1,4,14,2);
    vaStatLast = millis();
  }

  if(displayLeds) {
    displayLeds = false;
    FastLED.show();
  }
}
