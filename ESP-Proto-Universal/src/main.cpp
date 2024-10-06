//ESP32(Vin), all LEDs and fan is wired with 5V
//Gyro,OLED,Microphone,INA219 and touch is wired with 3.3V (gyro and mic needs RC filter)
//IF "configCRC.txt" doesn't get automaticaly generated before building filesystem, run "genCRC_manual.py" by hand and then build filesystem etc... again. (needs python installed)
//IF your WS leds do not correspond to set color, check color order for each strip in setup():FastLED.addLeds...
//Replace 0.1R with 0.03R resistor on INA219 board

#if defined(ESP32S3) //ESP32-S3
  #define MICpin 1 //Microphone
  #define T_in 2 //Output from Touch Sensor
  #define T_en 42 //Enable pin to Touch Sensor
  #define DATA_PIN_EARS 5  //Ears (from outer to inner, right cheek first)
  #define DATA_PIN_BLUSH 6 //Blush (from top to bottom, right cheek nearest to ear first) 
  #define DATA_PIN_VISOR 7 //Face (right cheek, left segment of eye first)
  #define I2C_SDA 8 //SDA for Gyro, OLED, INA219
  #define I2C_SCL 9 //SCL for Gyro, OLED, INA219
  #define MAX_CLK 12 //Clock for MAX72xx matrixes if used
  #define MAX_MOSI 11 //Data for MAX72xx matrixes if used
  #define MAX_CS 10 //ChipSelect for MAX72xx matrixes if used
  #define animBtn 4 //Pulling this pin LOW cycles trough animations
#elif defined(ESP32) //Normal ESP32 pins
  #define MICpin 35 //Microphone
  #define T_in 33 //Output from Touch Sensor
  #define T_en 23 //Enable pin to Touch Sensor
  #define DATA_PIN_EARS 5  //Ears (from outer to inner, right cheek first)
  #define DATA_PIN_BLUSH 18 //Blush (from top to bottom, right cheek nearest to ear first) 
  #define DATA_PIN_VISOR 19 //Face (right cheek, left segment of eye first)
  #define I2C_SDA 21 //SDA for Gyro, OLED, INA219
  #define I2C_SCL 22 //SCL for Gyro, OLED, INA219
  #define MAX_CLK 18 //Clock for MAX72xx matrixes if used
  #define MAX_MOSI 23 //Data for MAX72xx matrixes if used
  #define MAX_CS 5 //ChipSelect for MAX72xx matrixes if used
  #define animBtn 32 //Pulling this pin LOW cycles trough animations
#endif

#define wifi_en 13 //Pulling this pin LOW disables WiFi

//LEDs amount
#define MAX72xx_DEVICES 11
#define EarLedsNum 74
#define VisorLedsNum 704
#define blushLedsNum 8

#define MaxFEars 30 //Max amount of Ear frames (hardcoded to assign memory)
#define MaxFVisor 30 //Max amount of Visor frames

#define revertTilt 8000 //The maximum time that animation caused by tilt gets shown (used as if tilt bugs out etc)

//--------------------------------//No touching after this

#include <ezButton.h>
ezButton hwBtn(animBtn);

#include <StreamUtils.h>
#include <sstream>
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#define ARDUINOJSON_USE_DOUBLE 0
#include <ArduinoJson.h>

#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
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

//--------------------------------//CRC checksum func
#include <FastCRC.h>
class CrcWriter {
public:
  CrcWriter() {
    _hash = _hasher.crc32(NULL, 0);
  }

  size_t write(uint8_t c) {
    _hash = _hasher.crc32_upd(&c, 1);
    return 1;
  }

  size_t write(const uint8_t *buffer, size_t length) {
    _hash = _hasher.crc32_upd(buffer, length);
    return length;
  }

  uint32_t hash() const {
    return _hash;
  }

private:
  FastCRC32 _hasher;
  uint32_t _hash;
};

//--------------------------------//web / wifi
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <ElegantOTA.h>

AsyncWebServer server(80);

String wifiName = "Proto", wifiPass = "Proto123";

//--------------------------------//BLE
#define CONFIG_BT_NIMBLE_MAX_CONNECTIONS 2
#define CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED
#define CONFIG_BT_NIMBLE_ROLE_OBSERVER_DISABLED
#include "NimBLEDevice.h"

//add to config
String animToLoad = "";
String BLEfiles[20];
uint8_t BLEnum;

BLEServer *pServer = NULL;
BLECharacteristic * pCharacteristic;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
      String temp = String(pCharacteristic->getValue().c_str());
      if(temp.charAt(0) == 'g') { //legacy remote reasons
        pCharacteristic->setValue("i"+String(BLEnum));
        pCharacteristic->notify(true);
      } else if (temp.charAt(0) == '?') {
        String animtemp;
        for(int i = 0; i < BLEnum; i++) {
          animtemp += BLEfiles[i].substring(0, BLEfiles[i].length() - 5);
          animtemp += ";";
        }
        pCharacteristic->setValue(animtemp);
        pCharacteristic->notify(true);
      } else if (temp.toInt() > 0 && temp.toInt() <= BLEnum){
        animToLoad = BLEfiles[temp.toInt()-1];
      } else {
        for(int i = 0; i < BLEnum; i++) {
          if(temp == BLEfiles[i].substring(0, BLEfiles[i].length() - 5)) {
            animToLoad = BLEfiles[i];
          }
        }
      }
      Serial.println(temp);
    };
};

bool startBLE() {
  std::string stdStr(wifiName.c_str(), wifiName.length());
  BLEDevice::init(stdStr);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

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

//--------------------------------//getting stored anims names and count
String getfilesCache;
bool getfilesProper = true;

void getFilesFunc() {
  String temp;
  BLEnum = 0;
  File root = LittleFS.open("/anims");
  File file = root.openNextFile();
  while(file){
    temp += String(file.name()) + ";";
    BLEfiles[BLEnum] = String(file.name());
    BLEnum++;
    file = root.openNextFile();
  }
  getfilesCache = temp;
  getfilesProper = false;
}

//--------------------------------//MAX LEDs
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, MAX_MOSI, MAX_CLK, MAX_CS, MAX72xx_DEVICES);

//--------------------------------//WS2812 LEDs
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

//--------------------------------//Structs for anims in psram
struct FramesEars {
  int timespan;
  long ledColor[EarLedsNum];
};

struct AnimNowEars {
  int type;
  int numOfFrames;
  FramesEars frames[MaxFEars]; //max amount of ear frames
};

struct FramesVisor {
  int timespan;
  uint64_t leds[20];
  long ledsBlush[8];
};

struct AnimNowVisor {
  int type;
  int numOfFrames;
  FramesVisor frames[MaxFVisor]; //max amount of visor frames
  bool isMouth[20];
};

AnimNowEars* earsNow;
AnimNowVisor* visorNow;

//--------------------------------//Print centered text to oled buffer
void displayCenter(String text, uint16_t h) {
  float width = u8g2.getStrWidth(text.c_str());
  u8g2.drawStr((128 - width) / 2, h, text.c_str());
}

//--------------------------------//Config vars
bool speechEna = false, boopEna = false, tiltEna = false, bleEna = false, oledEna = false, instantReload = false, oledInitDone = false, tiltInitDone = false;
int rbSpeed, rbWidth, spMin, spMax, spTrig, currentEarsFrame = 0, currentVisorFrame = 0, bEar, bVisor, bBlush, numOfSegm, numAnimBlush;
String aTilt = "confused.json", aUp = "upset.json", currentAnim = "", visColorStr = "", ledType = "";
float neutralX, neutralY, neutralZ, tiltX, tiltY, tiltZ, upX, upY, upZ, tiltTol = 0.1;
unsigned long visColor = 0xFF0000;

String earTypes[5] = {"custom","rainbow","white_noise","corner_sabers","custom_glow"};
String visorTypes[2] = {"custom","all_rainbow"};

//--------------------------------//Load functions
bool loadAnim(String anim, String temp) {
  JsonDocument doc;
  DeserializationError error;

  if (currentAnim != anim) {
    if(anim == "POSTAnimLoad") {
      Serial.println("POST");
      error = deserializeJson(doc, temp);
    } else {
      File file = LittleFS.open("/anims/"+anim, "r");
      if (!file) {
        Serial.println("[E] There was an error opening the animation file!");
        file.close();
        return false;
      }
      Serial.println("[I] Animation file opened!");
      ReadBufferingStream bufferedFile{file, 64};
      error = deserializeJson(doc, bufferedFile);
      file.close();
    }
    
    if(error){
      Serial.println("[E] Failed to deserialize animation file!");
      Serial.println(error.c_str());
      return false;
    }

    currentAnim = anim;

    //Ears anim type
    for(int o=0;o<5;o++) {
      if(doc["ears"]["type"].as<String>() == earTypes[o]) {
        earsNow->type = o;
        break;
      }
    }
    //Ears anim load
    earsNow->numOfFrames = doc["ears"]["frames"].size();
    for(int x = 0; x < earsNow->numOfFrames; x++) {
      earsNow->frames[x].timespan = doc["ears"]["frames"][x]["timespan"].as<int>();
      for(int y = 0; y < doc["ears"]["frames"][x]["leds"].size(); y++) {
        earsNow->frames[x].ledColor[y] = strtol(doc["ears"]["frames"][x]["leds"][y].as<String>().c_str(), NULL, 16);
      }
    }

    //Visor anim type
    for(int o=0;o<2;o++) {
      if(doc["visor"]["type"].as<String>() == visorTypes[o]) {
        visorNow->type = o;
        break;
      }
    }
     //isMouth
    for(int y = 0; y < doc["visor"]["isMouth"].size(); y++) {
      visorNow->isMouth[y] = doc["visor"]["isMouth"][y].as<bool>();
    }
    //Visor anim load
    visorNow->numOfFrames = doc["visor"]["frames"].size();
    for(int x = 0; x < visorNow->numOfFrames; x++) {
      visorNow->frames[x].timespan = doc["visor"]["frames"][x]["timespan"].as<int>();
      numOfSegm = doc["visor"]["frames"][x]["leds"].size();
      for(int y = 0; y < doc["visor"]["frames"][x]["leds"].size(); y++) {
        visorNow->frames[x].leds[y] = strtoull(String(doc["visor"]["frames"][x]["leds"][y].as<String>()).c_str(), NULL, 16); //string to uint64
      }
      numAnimBlush = doc["visor"]["frames"][x]["ledsBlush"].size();
      for(int y = 0; y < doc["visor"]["frames"][x]["ledsBlush"].size(); y++) {
        visorNow->frames[x].ledsBlush[y] = strtol(doc["visor"]["frames"][x]["ledsBlush"][y].as<String>().c_str(), NULL, 16);
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

  return false;
}

bool loadConfig() {
  JsonDocument doc;

  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    Serial.println("[E] There was an error opening the config file, using default config");
    //using default
    return false;
  }
  Serial.println("[I] Config file opened");

  if(deserializeJson(doc, file)){
    Serial.println("[E] Failed to deserialize the config file, using default config");
    //using default
    return false;
  }
  file.close();
  Serial.println("[I] Deserialized JSON");
  
  CrcWriter CRCchk;
  serializeJson(doc, CRCchk);
  File fileCRC = LittleFS.open("/configCRC.txt", "r");
  if(!fileCRC) {
    Serial.println("[E] There was an error opening the config CRC file, using default config");
    fileCRC.close();
    return false;
  } else {
    String jsonCRC;
    while(fileCRC.available()) {
      jsonCRC+=char(fileCRC.read());
    }
    fileCRC.close();
    Serial.println("[I] CRC is: " + String(CRCchk.hash()) + " and should be: " + jsonCRC);
    if(jsonCRC == String(CRCchk.hash())) {
      Serial.println("[I] CRC check OK");
    } else {
      Serial.println("[E] CRC not the same, using default config");
      return false;
    }
  }
  
  boopEna = doc["boopEna"].as<bool>();
  speechEna = doc["speechEna"].as<bool>();
  tiltEna = doc["tiltEna"].as<bool>();
  bleEna = doc["bleEna"].as<bool>();
  oledEna = doc["oledEna"].as<bool>();
  bEar = doc["bEar"].as<int>();
  bVisor = doc["bVisor"].as<int>();
  bBlush = doc["bBlush"].as<int>();
  rbSpeed = doc["rbSpeed"].as<int>();
  rbWidth = doc["rbWidth"].as<int>();
  spMin = doc["spMin"].as<int>();
  spMax = doc["spMax"].as<int>();
  spTrig = doc["spTrig"].as<int>();
  aTilt = doc["aTilt"].as<String>();
  aUp = doc["aUp"].as<String>();
  neutralX = doc["neutralX"].as<float>();
  neutralY = doc["neutralY"].as<float>();
  neutralZ = doc["neutralZ"].as<float>();
  tiltX = doc["tiltX"].as<float>();
  tiltY = doc["tiltY"].as<float>();
  tiltZ = doc["tiltZ"].as<float>();
  upX = doc["upX"].as<float>();
  upY = doc["upY"].as<float>();
  upZ = doc["upZ"].as<float>();
  tiltTol = doc["tiltTol"].as<float>();
  visColorStr = doc["visColor"].as<String>();
  visColor = strtol(visColorStr.c_str()+1, NULL, 16);
  wifiName = doc["wifiName"].as<String>();
  wifiPass = doc["wifiPass"].as<String>();
  ledType = doc["ledType"].as<String>();

  return true;
}

bool saveConfig() {
  JsonDocument doc;

  File file = LittleFS.open("/config.json", "w");
  if (!file) {
    Serial.println("[E] There was an error opening the config file!");
    file.close();
    return false;
  }
  Serial.println("[I] Config file opened to save!");

  doc["boopEna"] = boopEna;
  doc["speechEna"] = speechEna;
  doc["tiltEna"] = tiltEna;
  doc["bleEna"] = bleEna;
  doc["oledEna"] = oledEna;
  doc["bEar"] = bEar;
  doc["bVisor"] = bVisor;
  doc["bBlush"] = bBlush;
  doc["rbSpeed"] = rbSpeed;
  doc["rbWidth"] = rbWidth;
  doc["spMin"] = spMin;
  doc["spMax"] = spMax;
  doc["spTrig"] = spTrig;
  doc["aTilt"] = aTilt;
  doc["aUp"] = aUp;
  doc["neutralX"] = neutralX;
  doc["neutralY"] = neutralY;
  doc["neutralZ"] = neutralZ;
  doc["tiltX"] = tiltX;
  doc["tiltY"] = tiltY;
  doc["tiltZ"] = tiltZ;
  doc["upX"] = upX;
  doc["upY"] = upY;
  doc["upZ"] = upZ;
  doc["tiltTol"] = tiltTol;
  doc["visColor"] = visColorStr;
  doc["wifiName"] = wifiName;
  doc["wifiPass"] = wifiPass;
  doc["ledType"] = ledType;
  
  CrcWriter CRCchk;
  serializeJson(doc, CRCchk);
  Serial.println("[I] Config CRC is: "+ String(CRCchk.hash()));
  File fileCRC = LittleFS.open("/configCRC.txt", "w");
  if(!fileCRC) {
    Serial.println("[E] There was an error opening the config CRC file!");
    fileCRC.close();
  } else {
    fileCRC.print(String(CRCchk.hash()));
    fileCRC.close();
    Serial.println("[I] CRC saved");
  }

  if(serializeJson(doc, file) == 0) {
    Serial.println("[E] Failed to deserialize the config file");
    return false;
  }

  file.close();
  Serial.println("[I] Config file saved with CRC");

  return true;
}

void setDefault() {
  boopEna = true;
  speechEna = true;
  tiltEna = false;
  bleEna = false;
  oledEna = false;
  bEar = 60;
  bVisor = 100;
  bBlush = 100;
  rbSpeed = 15;
  rbWidth = 8;
  spMin = 90;
  spMax = 110;
  spTrig = 1500;
  aTilt = "confused.json";
  aUp = "sad.json";
  neutralX = 0.3;
  neutralY = 0.92;
  neutralZ = 0.33;
  tiltX = 0.05;
  tiltY = 0.7;
  tiltZ = 0.73;
  upX = -0.17;
  upY = 0.99;
  upZ = 0.11;
  tiltTol = 0.1;
  visColorStr = "#ff0000";
  visColor = strtol(visColorStr.c_str()+1, NULL, 16);
  wifiName = "Proto";
  wifiPass = "Proto123";
  ledType = "WS2812";
  saveConfig();
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

//--------------------------------//Float mapping
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//--------------------------------//WiFi server setup
void startWiFiWeb() {
  WiFi.softAP(wifiName, wifiPass);

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.on("/getfiles", HTTP_GET, [](AsyncWebServerRequest *request){ //returns current anim + all avaible anims
    if(getfilesProper) {
      getFilesFunc();
    }
    request->send(200, "text/plain", currentAnim+";"+getfilesCache);
  });

  server.on("/saveconfig", HTTP_GET, [](AsyncWebServerRequest *request){ //saves config
    //enabled or disabled features
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
    //brightness
    if(request->hasParam("bEar"))
      bEar = request->getParam("bEar")->value().toInt();
    if(request->hasParam("bVisor"))
      bVisor = request->getParam("bVisor")->value().toInt();
    if(request->hasParam("bBlush"))
      bBlush = request->getParam("bBlush")->value().toInt();
    //anims configs
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
    //neutral tilt
    if(request->hasParam("neutralX"))
      neutralX = request->getParam("neutralX")->value().toFloat();
    if(request->hasParam("neutralY"))
      neutralY = request->getParam("neutralY")->value().toFloat();
    if(request->hasParam("neutralZ"))
      neutralZ = request->getParam("neutralZ")->value().toFloat();
    //side tilt
    if(request->hasParam("tiltX"))
      tiltX = request->getParam("tiltX")->value().toFloat();
    if(request->hasParam("tiltY"))
      tiltY = request->getParam("tiltY")->value().toFloat();
    if(request->hasParam("tiltZ"))
      tiltZ = request->getParam("tiltZ")->value().toFloat();
    //up tilt
    if(request->hasParam("upX"))
      upX = request->getParam("upX")->value().toFloat();
    if(request->hasParam("upY"))
      upY = request->getParam("upY")->value().toFloat();
    if(request->hasParam("upZ"))
      upZ = request->getParam("upZ")->value().toFloat();
    //tilt tolerant
    if(request->hasParam("tiltTol"))
      tiltTol = request->getParam("tiltTol")->value().toFloat();
    //color
    if(request->hasParam("visColor"))
      visColorStr = String(request->getParam("visColor")->value());
      visColor = strtol(visColorStr.c_str()+1, NULL, 16);
    //wifi
    if(request->hasParam("wifiName"))
      wifiName = String(request->getParam("wifiName")->value());
    if(request->hasParam("wifiPass"))
      wifiPass = String(request->getParam("wifiPass")->value());
    //led type
    if(request->hasParam("ledType"))
      ledType = String(request->getParam("ledType")->value());
    if(saveConfig()) {
      request->redirect("/saved.html?main");
    } else {
      request->send(200, "text/plain", "Saving config failed!");
    }
  });

  server.on("/savefile", HTTP_POST, [](AsyncWebServerRequest *request){ //saves data from POST to file
    if(request->hasParam("file", true) && request->hasParam("content", true)) {
      File file = LittleFS.open("/anims/"+request->getParam("file", true)->value()+".json", "w");
      if (!file) {
        Serial.println("There was an error opening the file for saving a animation!");
        file.close();
        request->send(200, "text/plain", "Error opening file for writing!");
      } else {
        Serial.println("File saved!");
        file.print(request->getParam("content", true)->value());
        file.close();
        getfilesProper = true;
        request->redirect("/saved.html?anim");
      }
    } else {
      request->send(200, "text/plain", "No valid parameters detected!");
    }
  });

  server.on("/deletefile", HTTP_GET, [](AsyncWebServerRequest *request){ //deletes asked file
    if(request->hasParam("file")) {
      LittleFS.remove("/anims/"+request->getParam("file")->value());
      getfilesProper = true;
      request->redirect("/saved.html?main");
    } else {
      request->send(200, "text/plain", "Parameter 'file' not present!");
    }
  });

  server.on("/change", HTTP_GET, [](AsyncWebServerRequest *request){ //loads anim from selected avaible anims
    if(request->hasParam("anim")) {
      if(loadAnim(request->getParam("anim")->value(),"")) {
        request->redirect("/saved.html?main");
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
        request->redirect("/saved.html?main");
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
  
  server.on("/gyro", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(floor(myIMU.readFloatAccelX()*100)/100)+";"+String(floor(myIMU.readFloatAccelY()*100)/100)+";"+String(floor(myIMU.readFloatAccelZ()*100)/100));
  });

  server.onNotFound([](AsyncWebServerRequest *request){request->send(404, "text/plain", "Not found");});
  ElegantOTA.begin(&server);
  server.begin();

  ElegantOTA.setAutoReboot(true);
}

//--------------------------------//OLED Init
void initOled() {
  Wire.begin();
  Wire.beginTransmission(60); //check for oled on address 0x3c
  byte error = Wire.endTransmission();
  if(error == 0) {
    u8g2.setBusClock(1500000);
    u8g2.begin();
    u8g2.setFlipMode(2);
    u8g2.setFont(u8g2_font_t0_22b_tf);
    u8g2.drawFrame(14, 36, 100, 9);
    if (!ina219.begin()) {
      Serial.println("[E] An Error has occurred while finding INA219 chip!");
    } else {
      ina219.setCalibration_16V_8A();
    }
    oledInitDone = true;
  } else {
    Serial.println("[E] An Error has occurred while initializing SSD1306.");
    oledEna = false;
  }
}

//--------------------------------//Setup
void setup() {
  Serial.begin(115200);

  pinMode(MICpin, INPUT);
  pinMode(T_in, INPUT_PULLUP);
  pinMode(T_en, OUTPUT);
  pinMode(wifi_en, INPUT_PULLUP);
  pinMode(animBtn, INPUT_PULLUP);

  hwBtn.setDebounceTime(50);

  if(!LittleFS.begin(true)) {
    Serial.println("[E] An Error has occurred while mounting LittleFS! Halting");
    while(1){};
  }

  if(psramInit() && ESP.getFreePsram() != 0) {
    earsNow = (AnimNowEars *)ps_malloc(sizeof(AnimNowEars));
    visorNow = (AnimNowVisor *)ps_malloc(sizeof(AnimNowVisor));
  } else {
    Serial.println("[E] Could not init PSRAM, either this ESP doesn't have one or is malfunctioning, halting...");
    while(1){};
  }

  if(!loadConfig()) {
    Serial.println("[E] An Error has occurred while loading config file! Loading defaults");
    setDefault();
  }

  ledController[0] = &FastLED.addLeds<WS2812B, DATA_PIN_EARS, GRB>(earLeds, EarLedsNum);
  ledController[1] = &FastLED.addLeds<WS2812B, DATA_PIN_BLUSH, RGB>(blushLeds, blushLedsNum);
  if(ledType == "WS2812") { //atm doing only visor, blush & ears stays on
    ledController[2] = &FastLED.addLeds<WS2812B, DATA_PIN_VISOR, GRB>(visorLeds, VisorLedsNum);
    FastLED.setCorrection(TypicalPixelString);
    FastLED.setDither(0);
  } else if (ledType == "MAX72XX") {
    mx.begin();
  }

  Wire.setPins(I2C_SDA, I2C_SCL);

  if(tiltEna) {
    if(myIMU.begin()) {
      Serial.println("[E] An Error has occurred while connecting to LSM!");
      tiltEna = false;
    } else {
      tiltInitDone = true;
    }
  }

  if(digitalRead(wifi_en) == HIGH) { //Pulling pin 13 LOW disables WiFi
    startWiFiWeb();
  }

  getFilesFunc();
  if(bleEna) { //you can disable BLE in config
    if(!startBLE()) {
      Serial.println("[E] An Error has occurred while starting BLE!");
    }
  }

  if(oledEna) {
    initOled();
  }

  Serial.println("[I] Free heap: "+String(ESP.getFreeHeap()));
  Serial.println("[I] Free PSRAM: "+String(ESP.getFreePsram()));

  loadAnim("default.json","");
}

//--------------------------------//Loop vars
String oldanim, boopoldanim;
bool booping = false, wasTilt = false, speechFirst = true, speechResetDone = false, speak = false, boopRea = false, displayLeds = false;
float zAx,yAx,finalMicAvg,nvol,micline,avgMicArr[10];
int randomNum, boopRead, randomTimespan = 0, startIndex = 1, speaking = 0, currentMicAvg = 0, btnNum = 0;
unsigned long lastMillsEars = 0, lastMillsVisor = 0, lastMillsTilt = 0, laskSpeakCheck = 0, lastSpeak = 0, lastMillsBoop = 0, lastMillsSpeechAnim = 0, lastFLED = 0, vaStatLast = 0, btnPressTime = 0, tiltChange = 0;
byte row = 0;

//--------------------------------//Visor bufferer
void setAllVisor(struct CRGB *ledArray, unsigned long ledColor, int visorFrame) {
  uint64_t tempSegment;
  for(int y = 0; y < numOfSegm; y++) {
    tempSegment = visorNow->frames[visorFrame].leds[y];
    if(visorNow->isMouth[y] && speak && randomNum == 0) { //if sets mouth and we are talking
      tempSegment = speakMatrix(tempSegment);
    }
    for (int i = 0; i < 8; i++) {
      row = (tempSegment >> i * 8) & 0xFF;
      for (int j = 0; j < 8; j++) {
        if(ledType == "WS2812") {
          ledArray[(y*64)+(i*8)+((i%2!=0)?j:7-j)] = (bitRead(row,j))?ledColor:CRGB::Black; //includes fix for bad rgbmatrix, to be fixed with new matrixes
        } else if (ledType == "MAX72XX") {
          mx.setPoint(i, j+(y*8), bitRead(row, j)); //MAXstuff
        }
      }
    }
  }
  fadeToBlackBy(visorLeds, VisorLedsNum, 255-bVisor);
  displayLeds = true;
}

bool isApproxEqual(const float ax, const float ay, const float az, const float bx, const float by, const float bz) {
  return (fabs(ax-bx) < tiltTol && fabs(ay-by) < tiltTol && fabs(az-bz) < tiltTol);
}  

void loop() {
  ElegantOTA.loop();
  //--------------------------------//EAR Leds render
  if(earsNow->type == 0) { //custom
    if(lastMillsEars+earsNow->frames[currentEarsFrame-1].timespan <= millis() || instantReload) {
      lastMillsEars = millis();
      if(currentEarsFrame == earsNow->numOfFrames) { currentEarsFrame = 0; } //loop back to first frame if last frame
      for(int y = 0; y < EarLedsNum; y++) { earLeds[y] = earsNow->frames[currentEarsFrame].ledColor[y]; } //set ear leds
      currentEarsFrame++;
      fadeToBlackBy(earLeds, EarLedsNum, 255-bEar);
      displayLeds = true;
    }
  } else if (earsNow->type == 1) { //rainbow
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
  } else if (earsNow->type == 2) { //white_noise
    memset(noiseData, 0, EarLedsNum);
    fill_raw_noise8(noiseData, EarLedsNum, 2, 0, 50, millis()/4);
    for(int x = 0;x<EarLedsNum;x++) {
      earLeds[x] = ColorFromPalette(blackWhite, noiseData[x]);
    }
    fadeToBlackBy(earLeds, EarLedsNum, 255-bEar);
    displayLeds = true;
  } else if (earsNow->type == 3) { //corner_sabers
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
  } else if (earsNow->type == 4) { //custom_glow
    fill_rainbow(pixelBuffer, 4, millis()/rbSpeed, 255/rbWidth);
    for(int y = 0; y < EarLedsNum; y++) {
      if(earsNow->frames[0].ledColor[y] == 0) {
        earLeds[y] = 0x000000;
      } else {
        earLeds[y] = pixelBuffer[0];
      }
    }
    fadeToBlackBy(earLeds, EarLedsNum, 255-bEar);
    displayLeds = true;
  }

  //--------------------------------//VISOR+BLUSH Leds render
  if(visorNow->type == 0) { //custom
    if(lastMillsVisor+visorNow->frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
      lastMillsVisor = millis();
      if(currentVisorFrame == visorNow->numOfFrames) { currentVisorFrame = 0; }
      setAllVisor(visorLeds,visColor,currentVisorFrame); //set visor leds
      for(int x = 0; x<8; x++) { blushLeds[x] = visorNow->frames[currentVisorFrame].ledsBlush[x]; } //set blush leds
      fadeToBlackBy(blushLeds, blushLedsNum, 255-bBlush);
      currentVisorFrame++;
      instantReload = false;
    }
  } else if (visorNow->type == 1 && ledType == "WS2812") { //all_rainbow
    if(lastMillsVisor+visorNow->frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
      lastMillsVisor = millis();
      if(currentVisorFrame == visorNow->numOfFrames) {
        currentVisorFrame = 0;
      }
    }
    fill_rainbow(visorPixelBuffer, 1, millis()/rbSpeed, 128/rbWidth);
    setAllVisor(visorLeds,((long)visorPixelBuffer[0].r << 16) | ((long)visorPixelBuffer[0].g << 8 ) | (long)visorPixelBuffer[0].b,currentVisorFrame);
    for(int x = 0; x<numAnimBlush; x++) { blushLeds[x] = visorNow->frames[currentVisorFrame].ledsBlush[x]; } //set blush leds
      fadeToBlackBy(blushLeds, blushLedsNum, 255-bBlush);
      if(lastMillsVisor+visorNow->frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
        currentVisorFrame++;
        instantReload = false;
      }
  }

  //--------------------------------//TILT
  if(lastMillsTilt+100<=millis() && tiltEna) {
    if(isApproxEqual(myIMU.readFloatAccelX(),myIMU.readFloatAccelY(),myIMU.readFloatAccelZ(),upX,upY,upZ) && !wasTilt) {
      Serial.println("up");
      wasTilt = true;
      oldanim = currentAnim;
      tiltChange = millis();
      loadAnim(aUp,"");
    } else if (isApproxEqual(myIMU.readFloatAccelX(),myIMU.readFloatAccelY(),myIMU.readFloatAccelZ(),tiltX,tiltY,tiltZ) && !wasTilt) {
      Serial.println("tilt");
      wasTilt = true;
      oldanim = currentAnim;
      tiltChange = millis();
      loadAnim(aTilt,"");
    } else if ((tiltChange+revertTilt<millis() || isApproxEqual(myIMU.readFloatAccelX(),myIMU.readFloatAccelY(),myIMU.readFloatAccelZ(),neutralX,neutralY,neutralZ)) && wasTilt) {
      Serial.println("neutral");
      wasTilt = false;
      loadAnim(oldanim,"");
    }
    lastMillsTilt = millis();
  } else if (!tiltInitDone && tiltEna) {
    if(myIMU.begin()) {
      Serial.println("[E] An Error has occurred while connecting to LSM!");
      tiltEna = false;
    } else {
      tiltInitDone = true;
    }
  }

  //--------------------------------//SPEECH Detection
  if(speechEna) {
    finalMicAvg = 0;
    nvol = 0;
    for (int i = 0; i<64; i++){
      micline = abs(analogRead(MICpin) - 512);
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
    if(visorNow->type == 0) { //custom
      setAllVisor(visorLeds,visColor,currentVisorFrame-1);
    }
    
    lastMillsSpeechAnim = millis();
  }
  //--------------------------------//SPEECH Reset frames
  if(!speechResetDone && !speak && lastMillsSpeechAnim+800<millis()) {
    if(visorNow->type == 0) { //custom
      setAllVisor(visorLeds,visColor,currentVisorFrame-1);
    } else if (visorNow->type == 1) { //all_rainbow
      setAllVisor(visorLeds,((long)visorPixelBuffer[0].r << 16) | ((long)visorPixelBuffer[0].g << 8 ) | (long)visorPixelBuffer[0].b,currentVisorFrame-1);
    }
    speechResetDone = true;
  }

  //--------------------------------//Single button anim change
  hwBtn.loop();
  if(hwBtn.isPressed()) { //detect press
    btnPressTime = millis();
  }
  if(hwBtn.isReleased()) {
    if(millis()-btnPressTime < 1500) { //short press
      btnNum++;
      if(btnNum >= BLEnum) {
        btnNum = 0;
      } else {
        animToLoad = BLEfiles[btnNum];
      }
      Serial.println("Changing to "+BLEfiles[btnNum]+", amount of anims: "+String(BLEnum));
    }
  }

  //--------------------------------//BOOP Detection
  if(millis() > 200 && boopEna) {
    digitalWrite(T_en, HIGH);
    delayMicroseconds(210);
    if(booping == false && !digitalRead(T_in)) {
      delayMicroseconds(395);
      if(!digitalRead(T_in)) {
        Serial.println("BOOP");
        booping = true;
        boopoldanim = currentAnim;
        loadAnim("boop.json","");
        lastMillsBoop = millis();
      }
      digitalWrite(T_en, LOW);
    } else if(booping == true && lastMillsBoop+1000<millis() && digitalRead(T_in)) {
      digitalWrite(T_en, LOW);
      Serial.println("unBOOP");
      booping = false;
      if(!wasTilt) {
        loadAnim(boopoldanim,"");
      }
    }
  }

  //--------------------------------//OLED routine, make for multiple sizes
  if(oledEna && vaStatLast+500<millis() && oledInitDone) {
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
  } else if (!oledInitDone && oledEna) {
    initOled();
  }

  if(displayLeds) {
    displayLeds = false;
    FastLED.show();
    if(ledType == "WS2812") {
      //FastLED.show(); //doing it before because ears and blush
    } else if (ledType == "MAX72XX") {
      mx.control(MD_MAX72XX::INTENSITY, bVisor);
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    }
  }

  if(animToLoad != "") {
    loadAnim(animToLoad,"");
    animToLoad = "";
    wasTilt = false;
    booping = false;
  }
}
