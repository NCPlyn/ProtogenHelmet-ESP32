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
  #define fanPWM 13 //PWM pin to control 4pin fan
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
  #define fanPWM 25 //PWM pin to control 4pin fan
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

#define oldMatrixFix true

#define oledAddr 60 //define oled on address 0x3c

//--------------------------------//No touching after this

#include <Arduino.h>

#define earTypeSize 5
#define visTypeSize 2
String earTypes[earTypeSize] = {"custom","rainbow","white_noise","corner_sabers","custom_glow"};
String visorTypes[visTypeSize] = {"custom","all_rainbow"};

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

#include "fileOp.h" //CRC + Config variables store/save/load/default
Config cfg;

#include "Misc.h" //Misc/helping functions
Misc misc;

#include "SparkFunLSM6DS3.h"
#include <Wire.h>
LSM6DS3 myIMU;

#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0,/* reset=*/ U8X8_PIN_NONE);

#include <Adafruit_INA219.h> //edited library in this sketch (replace 0.1R with 0.03R resistor on the board)
Adafruit_INA219 ina219;

//--------------------------------//web / wifi
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <ElegantOTA.h>

AsyncWebServer server(80);

String wifiName = "Proto", wifiPass = "Proto123";

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
  long ledsBlush[blushLedsNum];
};

struct AnimNowVisor {
  int type;
  int numOfFrames;
  FramesVisor frames[MaxFVisor]; //max amount of visor frames
  bool isMouth[20];
};

AnimNowEars* earsNow;
AnimNowVisor* visorNow;

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
      } else if (temp.charAt(0) == ';') { //command
        if (temp.indexOf("rgb") > 0 && cfg.ledType == "WS2812") {
          visorNow->type++;
          if(visorNow->type == visTypeSize)
            visorNow->type = 0;
          //do OLED stuff
        } else if (temp.indexOf("set") > 0) {
          temp.remove(0,4);
          int test = temp.toInt();
          //change set variable and do OLED stuff
        }
      } else if (temp.toInt() > 0 && temp.toInt() <= BLEnum){ //legacy remote reasons
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
CRGB blushLeds[blushLedsNum];
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

//--------------------------------//Print centered text to oled buffer
void displayCenter(String text, uint16_t h) {
  float width = u8g2.getStrWidth(text.c_str());
  u8g2.drawStr((128 - width) / 2, h, text.c_str());
}

//--------------------------------//Config vars
bool instantReload = false;
bool oledInitDone = false, tiltInitDone = false;
int currentEarsFrame = 0, currentVisorFrame = 0, numOfSegm, numAnimBlush;
String currentAnim = "";

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
    for(int o=0;o<earTypeSize;o++) {
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
    for(int o=0;o<visTypeSize;o++) {
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

    if(cfg.oledEna && oledInitDone) {
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

//--------------------------------//OLED Brightness
void oledBright(int level) {
  if(cfg.oledEna && oledInitDone) {
    if(level == 0) { //dim
      u8g2.sendF("ca", 0x0d9, (15 << 4) | 0 );
      u8g2.sendF("ca", 0x0db, 0 << 4);
    } else if (level == 1) { //mid
      u8g2.sendF("ca", 0x0d9, (15 << 4) | 15 );
      u8g2.sendF("ca", 0x0db, 0 << 4);
    } else if (level == 2) { //normal
      u8g2.sendF("ca", 0x0d9, (15 << 4) | 15 );
      u8g2.sendF("ca", 0x0db, 7 << 4);
    }
  }
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
      std::istringstream(request->getParam("boopEna")->value().c_str()) >> std::boolalpha >> cfg.boopEna;
    if(request->hasParam("speechEna"))
      std::istringstream(request->getParam("speechEna")->value().c_str()) >> std::boolalpha >> cfg.speechEna;
    if(request->hasParam("tiltEna"))
      std::istringstream(request->getParam("tiltEna")->value().c_str()) >> std::boolalpha >> cfg.tiltEna;
    if(request->hasParam("bleEna"))
      std::istringstream(request->getParam("bleEna")->value().c_str()) >> std::boolalpha >> cfg.bleEna;
    if(request->hasParam("oledEna"))
      std::istringstream(request->getParam("oledEna")->value().c_str()) >> std::boolalpha >> cfg.oledEna;
    //brightness
    if(request->hasParam("bEar"))
      cfg.bEar = request->getParam("bEar")->value().toInt();
    if(request->hasParam("bVisor"))
      cfg.bVisor = request->getParam("bVisor")->value().toInt();
    if(request->hasParam("bBlush"))
      cfg.bBlush = request->getParam("bBlush")->value().toInt();
    if(request->hasParam("bOled"))
      cfg.bOled = request->getParam("bOled")->value().toInt();
      oledBright(cfg.bOled);
    //anims configs
    if(request->hasParam("rbSpeed"))
      cfg.rbSpeed = request->getParam("rbSpeed")->value().toInt();
    if(request->hasParam("rbWidth"))
      cfg.rbWidth = request->getParam("rbWidth")->value().toInt();
    if(request->hasParam("spMin"))
      cfg.spMin = request->getParam("spMin")->value().toInt();
    if(request->hasParam("spMax"))
      cfg.spMax = request->getParam("spMax")->value().toInt();
    if(request->hasParam("spTrig"))
      cfg.spTrig = request->getParam("spTrig")->value().toInt();
    if(request->hasParam("aTilt"))
      cfg.aTilt = String(request->getParam("aTilt")->value());
    if(request->hasParam("aUp"))
      cfg.aUp = String(request->getParam("aUp")->value());
    //neutral tilt
    if(request->hasParam("neutralX"))
      cfg.neutralX = request->getParam("neutralX")->value().toFloat();
    if(request->hasParam("neutralY"))
      cfg.neutralY = request->getParam("neutralY")->value().toFloat();
    if(request->hasParam("neutralZ"))
      cfg.neutralZ = request->getParam("neutralZ")->value().toFloat();
    //side tilt
    if(request->hasParam("tiltX"))
      cfg.tiltX = request->getParam("tiltX")->value().toFloat();
    if(request->hasParam("tiltY"))
      cfg.tiltY = request->getParam("tiltY")->value().toFloat();
    if(request->hasParam("tiltZ"))
      cfg.tiltZ = request->getParam("tiltZ")->value().toFloat();
    //up tilt
    if(request->hasParam("upX"))
      cfg.upX = request->getParam("upX")->value().toFloat();
    if(request->hasParam("upY"))
      cfg.upY = request->getParam("upY")->value().toFloat();
    if(request->hasParam("upZ"))
      cfg.upZ = request->getParam("upZ")->value().toFloat();
    //tilt tolerant
    if(request->hasParam("tiltTol"))
      cfg.tiltTol = request->getParam("tiltTol")->value().toFloat();
    //color
    if(request->hasParam("visColor"))
      cfg.visColorStr = String(request->getParam("visColor")->value());
      cfg.visColor = strtol(cfg.visColorStr.c_str()+1, NULL, 16);
    //wifi
    if(request->hasParam("wifiName"))
      cfg.wifiName = String(request->getParam("wifiName")->value());
    if(request->hasParam("wifiPass"))
      cfg.wifiPass = String(request->getParam("wifiPass")->value());
    //led type
    if(request->hasParam("ledType"))
      cfg.ledType = String(request->getParam("ledType")->value());
    if(cfg.save()) {
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

  server.on("/fanpwm", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("duty")) {
      int duty = request->getParam("duty")->value().toInt();
      if(duty < 256 && duty >= 0) {
        ledcWrite(0, duty);
        //ledcWrite(fanPWM, duty); //Arduino 3.x core
        cfg.fanDuty = duty;
        cfg.save();
        request->send(200, "text/plain", "Set PWM to: " + String(duty));
      } else {
        request->send(200, "text/plain", "Invalid duty cycle");
      }
    } else {
      request->send(200, "text/plain", "No valid parameters detected!");
    }
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
  Wire.beginTransmission(oledAddr); //check for oled on address 0x3c
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
    oledBright(cfg.bOled);
    oledInitDone = true;
  } else {
    Serial.println("[E] An Error has occurred while initializing SSD1306.");
    cfg.oledEna = false;
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
  pinMode(0, INPUT_PULLUP);
  pinMode(fanPWM, OUTPUT);

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

  if(!cfg.load()) {
    Serial.println("[E] An Error has occurred while loading config file! Loading defaults");
    cfg.setDefault();
  }

  ledcAttach(fanPWM, 25000, 8); //suport Arduino 3.x
  ledcWrite(fanPWM, cfg.fanDuty); //Arduino 3.x core
  //ledcSetup(0, 25000, 8); //For Arduino 2.x
  //ledcAttachPin(fanPWM, 0); //For Arduino 2.x
  //ledcWrite(0, cfg.fanDuty); //for Arduino 2.x

  ledController[0] = &FastLED.addLeds<WS2812B, DATA_PIN_EARS, GRB>(earLeds, EarLedsNum);
  ledController[1] = &FastLED.addLeds<WS2812B, DATA_PIN_BLUSH, RGB>(blushLeds, blushLedsNum);
  if(cfg.ledType == "WS2812") { //atm doing only visor, blush & ears stays on
    ledController[2] = &FastLED.addLeds<WS2812B, DATA_PIN_VISOR, GRB>(visorLeds, VisorLedsNum);
    FastLED.setCorrection(TypicalPixelString);
    FastLED.setDither(0);
  } else if (cfg.ledType == "MAX72XX") {
    mx.begin();
  }

  Wire.setPins(I2C_SDA, I2C_SCL);

  if(cfg.tiltEna) {
    if(myIMU.begin()) {
      Serial.println("[E] An Error has occurred while connecting to LSM!");
      cfg.tiltEna = false;
    } else {
      tiltInitDone = true;
    }
  }

  if(digitalRead(wifi_en) == HIGH) { //Pulling pin 13 LOW disables WiFi
    startWiFiWeb();
  }

  getFilesFunc();
  if(cfg.bleEna) { //you can disable BLE in config
    if(!startBLE()) {
      Serial.println("[E] An Error has occurred while starting BLE!");
    }
  }

  if(cfg.oledEna) {
    initOled();
  }

  Serial.println("[I] Free heap: "+String(ESP.getFreeHeap()));
  Serial.println("[I] Free PSRAM: "+String(ESP.getFreePsram()));

  loadAnim("default.json","");
}

//--------------------------------//Loop vars
String oldanim, boopoldanim;
bool booping = false, wasTilt = false, speechFirst = true, speechResetDone = false, speak = false, boopRea = false, displayLeds = false;
float zAx,yAx,finalMicAvg,micline,avgMicArr[10];
int randomNum, boopRead, randomTimespan = 0, startIndex = 1, speaking = 0, currentMicAvg = 0, btnNum = 0;
unsigned long lastMillsEars = 0, lastMillsVisor = 0, lastMillsTilt = 0, laskSpeakCheck = 0, lastSpeak = 0, lastMillsBoop = 0, lastMillsSpeechAnim = 0, lastFLED = 0, vaStatLast = 0, btnPressTime = 0, tiltChange = 0, check0button = 0, looptime = 0;
byte row = 0;

//--------------------------------//Visor bufferer
void setAllVisor(struct CRGB *ledArray, unsigned long ledColor, int visorFrame) {
  uint64_t tempSegment;
  for(int y = 0; y < numOfSegm; y++) {
    tempSegment = visorNow->frames[visorFrame].leds[y];
    if(visorNow->isMouth[y] && speak && randomNum == 0) { //if sets mouth and we are talking
      tempSegment = misc.speakMatrix(tempSegment);
    }
    for (int i = 0; i < 8; i++) {
      row = (tempSegment >> i * 8) & 0xFF;
      for (int j = 0; j < 8; j++) {
        if(cfg.ledType == "WS2812") {
          if(oldMatrixFix) {
            ledArray[(y*64)+(i*8)+((i%2!=0)?j:7-j)] = (bitRead(row,j))?ledColor:CRGB::Black; //includes fix for bad rgbmatrix, to be fixed with new matrixes
          } else {
            ledArray[(y*64)+(i*8)+j] = (bitRead(row,j))?ledColor:CRGB::Black;
          }
        } else if (cfg.ledType == "MAX72XX") {
          mx.setPoint(i, j+(y*8), bitRead(row, j)); //MAXstuff
        }
      }
    }
  }
  fadeToBlackBy(visorLeds, VisorLedsNum, 255-cfg.bVisor);
  displayLeds = true;
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
      fadeToBlackBy(earLeds, EarLedsNum, 255-cfg.bEar);
      displayLeds = true;
    }
  } else if (earsNow->type == 1) { //rainbow
    fill_rainbow(pixelBuffer, 4, millis()/cfg.rbSpeed, 255/cfg.rbWidth);
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
    fadeToBlackBy(earLeds, EarLedsNum, 255-cfg.bEar);
    displayLeds = true;
  } else if (earsNow->type == 2) { //white_noise
    memset(noiseData, 0, EarLedsNum);
    fill_raw_noise8(noiseData, EarLedsNum, 2, 0, 50, millis()/4);
    for(int x = 0;x<EarLedsNum;x++) {
      earLeds[x] = ColorFromPalette(blackWhite, noiseData[x]);
    }
    fadeToBlackBy(earLeds, EarLedsNum, 255-cfg.bEar);
    displayLeds = true;
  } else if (earsNow->type == 3) { //corner_sabers
    if(lastFLED+cfg.rbSpeed < millis()) {
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
      fadeToBlackBy(earLeds, EarLedsNum, 255-cfg.bEar);
      displayLeds = true;
    }
  } else if (earsNow->type == 4) { //custom_glow
    fill_rainbow(pixelBuffer, 4, millis()/cfg.rbSpeed, 255/cfg.rbWidth);
    for(int y = 0; y < EarLedsNum; y++) {
      if(earsNow->frames[0].ledColor[y] == 0) {
        earLeds[y] = 0x000000;
      } else {
        earLeds[y] = pixelBuffer[0];
      }
    }
    fadeToBlackBy(earLeds, EarLedsNum, 255-cfg.bEar);
    displayLeds = true;
  }

  //--------------------------------//VISOR+BLUSH Leds render
  if(visorNow->type == 0 || (visorNow->type == 1 && cfg.ledType == "MAX72XX")) { //custom
    if(lastMillsVisor+visorNow->frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
      lastMillsVisor = millis();
      if(currentVisorFrame == visorNow->numOfFrames) { currentVisorFrame = 0; }
      setAllVisor(visorLeds,cfg.visColor,currentVisorFrame); //set visor leds
      for(int x = 0; x<8; x++) { blushLeds[x] = visorNow->frames[currentVisorFrame].ledsBlush[x]; } //set blush leds
      fadeToBlackBy(blushLeds, blushLedsNum, 255-cfg.bBlush);
      currentVisorFrame++;
      instantReload = false;
    }
  } else if (visorNow->type == 1 && cfg.ledType == "WS2812") { //all_rainbow
    if(lastMillsVisor+visorNow->frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
      lastMillsVisor = millis();
      if(currentVisorFrame == visorNow->numOfFrames) { currentVisorFrame = 0; }
      currentVisorFrame++;
      instantReload = false;
    }
    fill_rainbow(visorPixelBuffer, 1, millis()/cfg.rbSpeed, 128/cfg.rbWidth);
    setAllVisor(visorLeds,((long)visorPixelBuffer[0].r << 16) | ((long)visorPixelBuffer[0].g << 8 ) | (long)visorPixelBuffer[0].b,currentVisorFrame-1);
    for(int x = 0; x<numAnimBlush; x++) { blushLeds[x] = visorNow->frames[currentVisorFrame-1].ledsBlush[x]; } //set blush leds
    fadeToBlackBy(blushLeds, blushLedsNum, 255-cfg.bBlush);
  }

  //--------------------------------//TILT
  if(lastMillsTilt+100<=millis() && cfg.tiltEna) {
    if(misc.isApproxEqual(myIMU.readFloatAccelX(),myIMU.readFloatAccelY(),myIMU.readFloatAccelZ(),cfg.upX,cfg.upY,cfg.upZ,cfg.tiltTol) && !wasTilt) {
      Serial.println("up");
      wasTilt = true;
      oldanim = currentAnim;
      tiltChange = millis();
      loadAnim(cfg.aUp,"");
    } else if (misc.isApproxEqual(myIMU.readFloatAccelX(),myIMU.readFloatAccelY(),myIMU.readFloatAccelZ(),cfg.tiltX,cfg.tiltY,cfg.tiltZ,cfg.tiltTol) && !wasTilt) {
      Serial.println("tilt");
      wasTilt = true;
      oldanim = currentAnim;
      tiltChange = millis();
      loadAnim(cfg.aTilt,"");
    } else if ((tiltChange+revertTilt<millis() || misc.isApproxEqual(myIMU.readFloatAccelX(),myIMU.readFloatAccelY(),myIMU.readFloatAccelZ(),cfg.neutralX,cfg.neutralY,cfg.neutralZ,cfg.tiltTol)) && wasTilt) {
      Serial.println("neutral");
      wasTilt = false;
      loadAnim(oldanim,"");
    }
    lastMillsTilt = millis();
  } else if (!tiltInitDone && cfg.tiltEna) {
    if(myIMU.begin()) {
      Serial.println("[E] An Error has occurred while connecting to LSM!");
      cfg.tiltEna = false;
    } else {
      tiltInitDone = true;
    }
  }

  //looptime = micros();
  //--------------------------------//SPEECH Detection
  if(cfg.speechEna) { //8.8ms qwq
    float nvol = 0;
    for (int i = 0; i<64; i++){
      micline = abs(analogRead(MICpin) - 512);
      nvol = max(micline, nvol);
    }
    if(currentMicAvg == 9) {
      currentMicAvg = 0;
    } else {
      avgMicArr[currentMicAvg++] = nvol;
    }
  }
  //Serial.println(">SPK1:"+String(micros()-looptime));
  //looptime = micros();
  
  if(cfg.speechEna && laskSpeakCheck+10<=millis()) {
    finalMicAvg = 0;
    for (int i = 0; i<10; i++){
      finalMicAvg+=avgMicArr[i];
    }
    if(finalMicAvg/10 > cfg.spTrig) {
      speaking++;
      lastSpeak = millis();
    }
    if(speaking > 4 && !speak) {
      speak = true;
      speechResetDone = false;
      if(cfg.oledEna && oledInitDone) {
        u8g2.drawStr(8, 62, "speak");
        u8g2.updateDisplayArea(0,6,8,2);
      }
      Serial.println("Speak");
    }
    if(lastSpeak+500<millis() && speak) {
      speak = false;
      speechFirst = true;
      if(cfg.oledEna && oledInitDone) {
        u8g2.setDrawColor(0);
        u8g2.drawBox(8, 48, 56, 16);
        u8g2.updateDisplayArea(0,6,8,2);
        u8g2.setDrawColor(1);
      }
      Serial.println("unSpeak");
      speaking = 0;
    }
    laskSpeakCheck = millis();
  }
  //--------------------------------//SPEECH Animation
  if(cfg.speechEna && speak && lastMillsSpeechAnim+randomTimespan<=millis()) {
    randomTimespan = random(cfg.spMin,cfg.spMax);
    if(speechFirst == true){
      randomNum = 0;
      speechFirst = false;
    } else {
      randomNum = random(2);
    }
    if(visorNow->type == 0) { //custom
      setAllVisor(visorLeds,cfg.visColor,currentVisorFrame-1);
    }
    lastMillsSpeechAnim = millis();
  }
  //--------------------------------//SPEECH Reset frames
  if(!speechResetDone && !speak && lastMillsSpeechAnim+800<millis()) {
    if(visorNow->type == 0) { //custom
      setAllVisor(visorLeds,cfg.visColor,currentVisorFrame-1);
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
  if(millis() > 200 && cfg.boopEna) {
    digitalWrite(T_en, HIGH);
    delayMicroseconds(210);
    if(booping == false && !digitalRead(T_in)) {
      delayMicroseconds(395);
      if(!digitalRead(T_in)) {
        Serial.println("[I] IR BOOP");
        booping = true;
        boopoldanim = currentAnim;
        loadAnim("boop.json","");
        lastMillsBoop = millis();
      }
      digitalWrite(T_en, LOW);
    } else if(booping == true && lastMillsBoop+1000<millis() && digitalRead(T_in)) {
      digitalWrite(T_en, LOW);
      Serial.println("[I] IR unBOOP");
      booping = false;
      if(!wasTilt) {
        loadAnim(boopoldanim,"");
      }
    }
  }

  //looptime = micros();
  //--------------------------------//OLED routine, make for multiple sizes, 10ms qwq
  if(cfg.oledEna && vaStatLast+500<millis() && oledInitDone) {
    float BusV = ina219.getBusVoltage_V();
    int barStatus = misc.mapfloat(BusV,5.5,8.42,0,100);
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
  } else if (!oledInitDone && cfg.oledEna) {
    initOled();
  }
  //Serial.println(">OLED:"+String(micros()-looptime));
  //looptime = micros();

  if(displayLeds) {
    displayLeds = false;
    FastLED.show();
    if(cfg.ledType == "WS2812") {
      //FastLED.show(); //doing it before because ears and blush
    } else if (cfg.ledType == "MAX72XX") {
      if(cfg.bVisor > 15) { cfg.bVisor = 15;}
      mx.control(MD_MAX72XX::INTENSITY, cfg.bVisor);
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    }
  }

  //press boot button for 10sec to reset
  if(check0button+10000 < millis() && check0button+10500 > millis() && digitalRead(0) == LOW) {
    Serial.println("[I] Resetting to defaults");
    cfg.setDefault();
    delay(20);
    ESP.restart();
  } else if (digitalRead(0) == HIGH && check0button+10000 > millis() && check0button < millis()) {
    check0button = 0;
  } else if (digitalRead(0) == LOW && check0button+10000 < millis() && check0button < millis()) {
    check0button = millis();
    Serial.println("[I] set def wait");
  }

  if(animToLoad != "") {
    loadAnim(animToLoad,"");
    animToLoad = "";
    wasTilt = false;
    booping = false;
  }
}
