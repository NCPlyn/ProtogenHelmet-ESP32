//Make sure you have everything connected by the schematic in the repository and set these defines correctly!

#define MICpin ADC_CHANNEL_0 //Microphone, pin 1
#define T_in 2 //Output from Touch Sensor
#define T_en 42 //Enable pin to Touch Sensor
#define DATA_PIN_EARS 5  //Ears(Blush) (from outer to inner, POV-right cheek, (if blush: from top to bottom, right cheek nearest to ear first))
#define DATA_PIN_VISOR 7 //Face (right cheek, left segment of eye first)
#define I2C_SDA 8 //SDA for Gyro, OLED, INA219
#define I2C_SCL 9 //SCL for Gyro, OLED, INA219
#define MAX_CLK 12 //Clock for MAX72xx matrixes if used
#define MAX_MOSI 11 //Data for MAX72xx matrixes if used
#define MAX_CS 10 //ChipSelect for MAX72xx matrixes if used
#define animBtn 4 //Pulling this pin LOW cycles trough animations
#define fanPWM 13 //PWM pin to control 4pin fan

#define visorType "WS2812" // What displays are you using? (WS2812 or MAX72XX so far)
#define MAX72xx_DEVICES 11 // How many MAX72xx matrices for visor?
#define visorLedsNum 704 // How many WS2812 LEDs for visor? (matrixNumber*64)
#define FADESTEPS 4 //how many steps when fading between frames? (0=disabled)

bool earPresent = true; // Are you using ear leds?
#define earLedsNum 74 // How many? (74 rn, no other option atm)

bool blushPresent = false; // Are you using blush leds?
#define blushLedsNum 8 // How many? (might crash under 8)
bool useRGBblush = true; //Swaps red-green for RGB strip

bool INApresent = true; //Are you using INA219?

#define boopMode "IR-KY" //"IR-KY" for KY-032, "Capac" for capacitive sensor/boop when HIGH, "IR-Dist" for ADPS... tbd

#define revertTilt 8000 //The maximum time that animation caused by tilt gets shown (used as if tilt bugs out etc)

#define oldMatrixFix false //fix for Legacy WS2812B-2020 matrix

#define oledAddr 60 //define oled on address 0x3c

//--------------------------------//No touching after this!

#define MaxFEars 30 //Max amount of Ear frames (hardcoded to assign memory)
#define MaxFVisor 30 //Max amount of Visor frames

#include <Arduino.h>

#include "esp_adc/adc_oneshot.h"
adc_oneshot_unit_handle_t adc_handle;

#define earTypeSize 5
#define visTypeSize 2
String earTypes[earTypeSize] = {"custom","rainbow","white_noise","corner_sabers","custom_glow"}; //available ear type animations
String visorTypes[visTypeSize] = {"custom","all_rainbow"}; //available visor type animations
String vTAcro[visTypeSize] = {"cust","rnbw"}; //OLED acronyms for visor type animations

#include <ezButton.h>
ezButton hwBtn(animBtn);

#include <StreamUtils.h>
#include <sstream>
#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_RMT5 = 0 //doesnt compile
#define FASTLED_ESP32_FLASH_LOCK 1
#include <FastLED.h>
#define ARDUINOJSON_USE_DOUBLE 0
#include <ArduinoJson.h>

#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#include <LittleFS.h>

#include "fileOp.h" //CRC + Config variables store/save/load/default
Config cfg;

#include "Misc.h" //Misc/helping functions
Misc misc;

#include "oled.h"
SSDOLED oled;

#include "SparkFunLSM6DS3.h"
#include <Wire.h>
LSM6DS3 myIMU;

#include <Adafruit_INA219.h> //edited library in this sketch (replace 0.1R with 0.03R resistor on the board)
Adafruit_INA219 ina219;

//--------------------------------//web / wifi
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <ElegantOTA.h>

AsyncWebServer server(80);

String wifiName = "ProtoWiFi", wifiPass = "Proto1234";

//--------------------------------//Config vars
bool instantReload = false, oledInitDone = false, tiltInitDone = false, getfilesProper = true;
uint8_t currentEarsFrame = 0, currentVisorFrame = 0, numOfSegm, numAnimBlush, totalAnims;
String currentAnim = "", animToLoad = "", availAnims[50], getfilesCache;

//--------------------------------//getting stored anims names and count
void getFilesFunc() {
  getfilesCache = "";
  totalAnims = 0;
  File root = LittleFS.open("/anims");
  File file = root.openNextFile();
  while(file){
    availAnims[totalAnims] = String(file.name());
    getfilesCache += availAnims[totalAnims] + ";";
    totalAnims++;
    file = root.openNextFile();
  }
  getfilesProper = false;
}

//--------------------------------//Structs for anims in psram
struct FramesEars {
  int timespan;
  long ledColor[earLedsNum];
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
  long fColor[(visorLedsNum/64)+1];
};

struct AnimNowVisor {
  int type;
  int numOfFrames;
  FramesVisor frames[MaxFVisor]; //max amount of visor frames
  bool isMouth[20];
};

AnimNowEars* earsNow;
AnimNowVisor* visorNow;

//--------------------------------//MAX LEDs
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, MAX_MOSI, MAX_CLK, MAX_CS, MAX72xx_DEVICES);

//--------------------------------//WS2812 LEDs
CRGB earLeds[earLedsNum];
CRGB blushLeds[blushLedsNum];
CRGB visorLeds[visorLedsNum];
CRGB visorLedsNEW[visorLedsNum];
CRGB c2Leds[earLedsNum+blushLedsNum];

CLEDController *ledController[2];

CRGB pixelBuffer[18];
CRGB visorPixelBuffer[10];
uint8_t noiseData[earLedsNum];

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

//--------------------------------//Load functions
bool loadAnim(String anim, String temp) {
  JsonDocument doc;
  DeserializationError error;

  if (currentAnim != anim || anim == "POSTAnimLoad") {
    if(anim == "POSTAnimLoad") {
      Serial.println(F("[I] POST load"));
      error = deserializeJson(doc, temp);
    } else {
      delay(25);
      File file = LittleFS.open("/anims/"+anim, "r");
      if (!file) {
        Serial.println(F("[E] There was an error opening the animation file!"));
        file.close();
        return false;
      }
      Serial.println(F("[I] Animation file opened!"));
      ReadBufferingStream bufferedFile{file, 64};
      error = deserializeJson(doc, bufferedFile);
      file.close();
    }
    
    if(error){
      Serial.print(F("[E] Failed to deserialize animation file! : "));
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
      numAnimBlush = min((uint8_t)doc["visor"]["frames"][x]["ledsBlush"].size(),numAnimBlush); //overflow fix
      for(int y = 0; y < numAnimBlush; y++) {
        visorNow->frames[x].ledsBlush[y] = strtol(doc["visor"]["frames"][x]["ledsBlush"][y].as<String>().c_str(), NULL, 16);
      }
      numOfSegm = doc["visor"]["frames"][x]["leds"].size();
      for(int y = 0; y < numOfSegm; y++) {
        visorNow->frames[x].fColor[y] = strtol(doc["visor"]["frames"][x]["fColor"][y].as<String>().c_str(), NULL, 16); //should return 0 if not present
        visorNow->frames[x].leds[y] = strtoull(doc["visor"]["frames"][x]["leds"][y].as<String>().c_str(), NULL, 16); //string to uint64
      }
    }

    instantReload = true;
    currentVisorFrame = 0;
    currentEarsFrame = 0;

    if(cfg.oledEna && oledInitDone) {
      oled.writeAnim(anim.substring(0,anim.length()-5));
      oled.writeRGB(vTAcro[visorNow->type]);
    }
    return true;
  }
  return false;
}

//--------------------------------//BLE
#define CONFIG_BT_NIMBLE_MAX_CONNECTIONS 2
#define CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED
#define CONFIG_BT_NIMBLE_ROLE_OBSERVER_DISABLED
#include "NimBLEDevice.h"

BLEServer *pServer = NULL;
BLECharacteristic * pCharacteristic;
BLEAdvertising* pAdvertising;

class MyCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
      String temp = String(pCharacteristic->getValue().c_str());
      if(temp.charAt(0) == 'g') { //legacy remote reasons
        pCharacteristic->setValue("i"+String(totalAnims));
        pCharacteristic->notify();
      } else if (temp.charAt(0) == '?') {
        String animtemp;
        for(int i = 0; i < totalAnims; i++) {
          animtemp += availAnims[i].substring(0, availAnims[i].length() - 5);
          animtemp += ";";
        }
        pCharacteristic->setValue(animtemp);
        pCharacteristic->notify(true);
      } else if (temp.charAt(0) == ';') { //command
        if (temp.indexOf("rgb") > 0 && visorType == "WS2812") {
          visorNow->type++;
          if(visorNow->type == visTypeSize)
            visorNow->type = 0;
          if(cfg.oledEna && oledInitDone)
            oled.writeRGB(vTAcro[visorNow->type]);
        } else if (temp.indexOf("set") > 0) {
          temp.remove(0,4);
          if(cfg.oledEna && oledInitDone)
            oled.writeSet(temp.toInt()+1);
        }
      } else if (temp.toInt() > 0 && temp.toInt() <= totalAnims){ //legacy remote reasons
        animToLoad = availAnims[temp.toInt()-1];
      } else {
        for(int i = 0; i < totalAnims; i++) {
          if(temp == availAnims[i].substring(0, availAnims[i].length() - 5)) {
            animToLoad = availAnims[i];
          }
        }
      }
      Serial.print(F("[I] BT Recv.: "));
      Serial.println(temp);
    };
} chrCallbacks;

class ServerCallbacks : public NimBLEServerCallbacks {
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
      NimBLEDevice::startAdvertising();
  }
} serverCallbacks;

bool startBLE() {
  std::string stdStr(wifiName.c_str(), wifiName.length());
  BLEDevice::init(stdStr);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(&serverCallbacks);

  BLEService *pService = pServer->createService("ffe0");

  pCharacteristic = pService->createCharacteristic("ffe1",
      NIMBLE_PROPERTY::BROADCAST | NIMBLE_PROPERTY::READ  |
      NIMBLE_PROPERTY::NOTIFY    | NIMBLE_PROPERTY::WRITE |
      NIMBLE_PROPERTY::INDICATE
  );
  pCharacteristic->setValue(totalAnims);
  pCharacteristic->setCallbacks(&chrCallbacks);

  if(!pService->start()) {
    return false;
  }

  pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName(stdStr);
  pAdvertising->addServiceUUID(BLEUUID(pService->getUUID()));
  pAdvertising->enableScanResponse(true);
  if(!pAdvertising->start(0)) {
    return false;
  }

  return true;
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
    if(request->hasParam("bOled"))
      cfg.bOled = request->getParam("bOled")->value().toInt();
      if(cfg.oledEna && oledInitDone) {
        oled.oledBright(cfg.bOled);
      }
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
    if(request->hasParam("aBoop"))
      cfg.aBoop = String(request->getParam("aBoop")->value());
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
    delay(25);
    if(cfg.save()) {
      instantReload = true;
      request->redirect("/saved.html?main");
    } else {
      request->send(200, "text/plain", F("Saving config failed!"));
    }
  });

  server.on("/savefile", HTTP_POST, [](AsyncWebServerRequest *request){ //saves data from POST to file
    if(request->hasParam("file", true) && request->hasParam("content", true)) {
      File file = LittleFS.open("/anims/"+request->getParam("file", true)->value()+".json", "w");
      if (!file) {
        Serial.println(F("[E] There was an error opening the file for saving an animation!"));
        file.close();
        request->send(200, "text/plain", F("Error opening file for writing!"));
      } else {
        Serial.println(F("[I] File saved!"));
        file.print(request->getParam("content", true)->value());
        file.close();
        getfilesProper = true;
        request->redirect("/saved.html?anim");
      }
    } else {
      request->send(200, "text/plain", F("No valid parameters detected!"));
    }
  });

  server.on("/deletefile", HTTP_GET, [](AsyncWebServerRequest *request){ //deletes asked file
    if(request->hasParam("file")) {
      LittleFS.remove("/anims/"+request->getParam("file")->value());
      getfilesProper = true;
      request->redirect("/saved.html?main");
    } else {
      request->send(200, "text/plain", F("Parameter 'file' not present!"));
    }
  });

  server.on("/change", HTTP_GET, [](AsyncWebServerRequest *request){ //loads anim from selected avaible anims
    if(request->hasParam("anim")) {
      if(request->getParam("anim")->value() == currentAnim) {
        request->redirect("/saved.html?main");
      } else if(loadAnim(request->getParam("anim")->value(),"")) {
        request->redirect("/saved.html?main");
      } else {
        request->send(200, "text/plain", F("Loading animation has failed!"));
      }
    } else {
      request->send(200, "text/plain", F("No valid parameters detected!"));
    }
  });

  server.on("/change", HTTP_POST, [](AsyncWebServerRequest *request){ //loads anim from POST request
    if(request->hasParam("anim", true)) {
      if(loadAnim("POSTAnimLoad",request->getParam("anim", true)->value())) {
        request->redirect("/saved.html?main");
      } else {
        request->send(200, "text/plain", F("Loading animation has failed!"));
      }
    } else {
      request->send(200, "text/plain", F("No valid parameters detected!"));
    }
  });

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/fanpwm", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("duty")) {
      int duty = request->getParam("duty")->value().toInt();
      if(duty < 256 && duty >= 0) {
        //ledcWrite(0, duty);
        ledcWrite(fanPWM, duty); //Arduino 3.x core
        cfg.fanDuty = duty;
        cfg.save();
        request->send(200, "text/plain", "Set PWM to: " + String(duty));
      } else {
        request->send(200, "text/plain", F("Invalid duty cycle"));
      }
    } else {
      request->send(200, "text/plain", F("No valid parameters detected!"));
    }
  });
  
  server.on("/rgb", HTTP_GET, [](AsyncWebServerRequest *request){
    if(visorType == "WS2812") {
      visorNow->type++;
      if(visorNow->type == visTypeSize)
        visorNow->type = 0;
      if(cfg.oledEna && oledInitDone)
        oled.writeRGB(vTAcro[visorNow->type]);
        Serial.println("[I] Changing visor type to: "+visorTypes[visorNow->type]);
    }
    request->redirect("/saved.html?main");
  });

  server.on("/gyro", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(floor(myIMU.readFloatAccelX()*100)/100)+";"+String(floor(myIMU.readFloatAccelY()*100)/100)+";"+String(floor(myIMU.readFloatAccelZ()*100)/100));
  });

  server.onNotFound([](AsyncWebServerRequest *request){request->send(404, "text/plain", "Not found");});
  ElegantOTA.begin(&server);
  server.begin();

  ElegantOTA.setAutoReboot(true);
}

//--------------------------------//Setup
void setup() {
  Serial.begin(115200);

  if(boopMode == "IR-KY") {
    pinMode(T_in, INPUT_PULLUP);
  } else {
    pinMode(T_in, INPUT_PULLDOWN);
  }
  pinMode(T_en, OUTPUT);
  pinMode(animBtn, INPUT_PULLUP);
  pinMode(0, INPUT_PULLUP);
  pinMode(fanPWM, OUTPUT);

  hwBtn.setDebounceTime(50);

  if(!LittleFS.begin(true)) {
    Serial.println(F("[E] An Error has occurred while mounting LittleFS! Halting"));
    while(1){};
  }

  if(psramInit() && ESP.getFreePsram() != 0) {
    earsNow = (AnimNowEars *)ps_malloc(sizeof(AnimNowEars));
    visorNow = (AnimNowVisor *)ps_malloc(sizeof(AnimNowVisor));
  } else {
    Serial.println(F("[E] Could not init PSRAM, either this ESP doesn't have one or is malfunctioning, halting..."));
    while(1){};
  }

  if(!cfg.load()) {
    Serial.println(F("[E] An Error has occurred while loading config file! Loading defaults"));
    cfg.setDefault();
  }

  adc_oneshot_unit_init_cfg_t init_config = {
    .unit_id = ADC_UNIT_1,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
  adc_oneshot_chan_cfg_t channel_config = {
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_12,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, MICpin, &channel_config));

  ledcAttach(fanPWM, 25000, 8); //suport Arduino 3.x
  ledcWrite(fanPWM, cfg.fanDuty); //Arduino 3.x core
  //ledcSetup(0, 25000, 8); //For Arduino 2.x
  //ledcAttachPin(fanPWM, 0); //For Arduino 2.x
  //ledcWrite(0, cfg.fanDuty); //for Arduino 2.x

  if(visorType == "WS2812") {
    ledController[0] = &FastLED.addLeds<WS2812B, DATA_PIN_VISOR, GRB>(visorLeds, visorLedsNum);
  } else if (visorType == "MAX72XX") {
    mx.begin();
  }
  if(earPresent || blushPresent) { //RMT4 2 controller fix
    ledController[1] = &FastLED.addLeds<WS2812B, DATA_PIN_EARS, GRB>(c2Leds, earLedsNum+blushLedsNum);
  }
  /*if(earPresent) {
    ledController[1] = &FastLED.addLeds<WS2812B, DATA_PIN_EARS, GRB>(earLeds, earLedsNum);
  }
  if(blushPresent) {
    ledController[2] = &FastLED.addLeds<WS2812B, DATA_PIN_BLUSH, RGB>(blushLeds, blushLedsNum);
  }*/
  FastLED.setCorrection(TypicalPixelString);
  FastLED.setDither(0);

  startWiFiWeb();

  getFilesFunc();
  if(cfg.bleEna) { //you can disable BLE in config
    if(!startBLE()) {
      Serial.println(F("[E] An Error has occurred while starting BLE!"));
    }
  }

  //I2C things
  Wire.setPins(I2C_SDA, I2C_SCL);

  if(cfg.tiltEna) {
    if(myIMU.begin()) {
      Serial.println(F("[E] An Error has occurred while connecting to LSM!"));
      cfg.tiltEna = false;
    } else {
      tiltInitDone = true;
    }
  }

  if(cfg.oledEna) {
    if(!oled.init(oledAddr,cfg.bOled,INApresent)) {
      Serial.println(F("[E] An Error has occurred while initializing SSD1306."));
      cfg.oledEna = false;
    } else {
      oledInitDone = true;
      oled.speak(false);
      oled.writeSet(1);
    }
  }

  if(INApresent && oledInitDone) {
    if(!ina219.begin()) {
      Serial.println(F("[E] An Error has occurred while finding INA219 chip!"));
      INApresent = false;
    } else {
      ina219.setCalibration_16V_8A();
    }
  }
  
  while(millis()<2000) {yield();} //2s delay for the anim to load properly (idk why but it doesnt without this or with 1s)
  loadAnim("default.json","");

  Serial.println("[I] Free heap: "+String(ESP.getFreeHeap()));
  Serial.println("[I] Free PSRAM: "+String(ESP.getFreePsram()));
}

//--------------------------------//Loop vars
String oldanim, boopoldanim;
bool FdisplayVisor = false, FdisplayBlush = false, FdisplayEar = false, booping = false, wasTilt = false, speechFirst = true, speechResetDone = false, speak = false, boopRea = false, remoteSign = false;;
float zAx,yAx,finalMicAvg,avgMicArr[10];
int randomNum, boopRead, randomTimespan = 0, startIndex = 1, speaking = 0, currentMicAvg = 0, btnNum = 0, currFade = 1;
unsigned long lastMillsEars = 0, lastMillsVisor = 0, lastMillsTilt = 0, laskSpeakCheck = 0, lastSpeak = 0, lastMillsBoop = 0, lastMillsSpeechAnim = 0, lastFLED = 0, vaStatLast = 0, btnPressTime = 0, tiltChange = 0, check0button = 0, looptime = 0, fadeTime = 0;
byte row = 0;

//--------------------------------//Visor bufferer
void setAllVisor(struct CRGB *ledArray, long ledColor, int visorFrame) {
  uint64_t tempSegment;
  for(int y = 0; y < numOfSegm; y++) {
    tempSegment = visorNow->frames[visorFrame].leds[y];
    if(visorNow->isMouth[y] && speak && randomNum == 0) { //if sets mouth and we are talking
      tempSegment = misc.speakMatrix(tempSegment);
    }
    for (int i = 0; i < 8; i++) {
      row = (tempSegment >> i * 8) & 0xFF;
      for (int j = 0; j < 8; j++) {
        if(visorType == "WS2812") {
          long tempColor = ledColor; //use given color
          if(ledColor == 0) { //if given color == 0, use config color
            tempColor = cfg.visColor;
            if(visorNow->frames[visorFrame].fColor[y] != 0) { //if theres color then 0 in anim, use that
              tempColor = visorNow->frames[visorFrame].fColor[y];
            }
          }
          if(oldMatrixFix) {
            ledArray[(y*64)+(i*8)+((i%2!=0)?j:7-j)] = (bitRead(row,j))?tempColor:CRGB::Black; //includes fix for bad rgbmatrix
          } else {
            ledArray[(y*64)+(i*8)+j] = (bitRead(row,j))?tempColor:CRGB::Black;
          }
        } else if (visorType == "MAX72XX") {
          mx.setPoint(i, j+(y*8), bitRead(row, j)); //MAXstuff
        }
      }
    }
  }
  FdisplayVisor = true;
  currFade = 1;
}

void loop() {
  ElegantOTA.loop();
  //--------------------------------//EAR Leds render
  if(earPresent) {
    if(earsNow->type == 0) { //custom
      if(lastMillsEars+earsNow->frames[currentEarsFrame-1].timespan <= millis() || instantReload) {
        lastMillsEars = millis();
        if(currentEarsFrame == earsNow->numOfFrames) { currentEarsFrame = 0; } //loop back to first frame if last frame
        for(int y = 0; y < earLedsNum; y++) { earLeds[y] = earsNow->frames[currentEarsFrame].ledColor[y]; } //set ear leds
        currentEarsFrame++;
        FdisplayEar = true;
      }
    } else if (earsNow->type == 1) { //rainbow
      fill_rainbow(pixelBuffer, 4, millis()/cfg.rbSpeed, 255/cfg.rbWidth);
      for(int x = 0;x<earLedsNum;x++) {
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
      FdisplayEar = true;
    } else if (earsNow->type == 2) { //white_noise
      memset(noiseData, 0, earLedsNum);
      fill_raw_noise8(noiseData, earLedsNum, 2, 0, 50, millis()/4);
      for(int x = 0;x<earLedsNum;x++) {
        earLeds[x] = ColorFromPalette(blackWhite, noiseData[x]);
      }
      FdisplayEar = true;
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
        FdisplayEar = true;
      }
    } else if (earsNow->type == 4) { //custom_glow
      fill_rainbow(pixelBuffer, 4, millis()/cfg.rbSpeed, 255/cfg.rbWidth);
      for(int y = 0; y < earLedsNum; y++) {
        if(earsNow->frames[0].ledColor[y] == 0) {
          earLeds[y] = 0x000000;
        } else {
          earLeds[y] = pixelBuffer[0];
        }
      }
      FdisplayEar = true;
    }
  }

  //--------------------------------//VISOR+BLUSH Leds render
  if(visorNow->type == 0 || (visorNow->type == 1 && visorType == "MAX72XX")) { //custom
    if(lastMillsVisor+visorNow->frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
      lastMillsVisor = millis();
      if(currentVisorFrame == visorNow->numOfFrames) { currentVisorFrame = 0; }
      setAllVisor(visorLedsNEW,0,currentVisorFrame); //set visor leds
      if(blushPresent) {
        for(int x = 0; x<8; x++) { blushLeds[x] = visorNow->frames[currentVisorFrame].ledsBlush[x]; } //set blush leds
      }
      FdisplayBlush = true;
      currentVisorFrame++;
      instantReload = false;
    }
  } else if (visorNow->type == 1 && visorType == "WS2812") { //all_rainbow
    if(lastMillsVisor+visorNow->frames[currentVisorFrame-1].timespan <= millis() || instantReload) {
      lastMillsVisor = millis();
      if(currentVisorFrame == visorNow->numOfFrames) { currentVisorFrame = 0; }
      currentVisorFrame++;
      instantReload = false;
    }
    fill_rainbow(visorPixelBuffer, 1, millis()/cfg.rbSpeed, 128/cfg.rbWidth);
    setAllVisor(visorLeds,((long)visorPixelBuffer[0].r << 16) | ((long)visorPixelBuffer[0].g << 8 ) | (long)visorPixelBuffer[0].b,currentVisorFrame-1);
    if(blushPresent) {
      for(int x = 0; x<numAnimBlush; x++) { blushLeds[x] = visorNow->frames[currentVisorFrame-1].ledsBlush[x]; } //set blush leds
    }
    FdisplayBlush = true;
  }

  //--------------------------------//TILT
  if(lastMillsTilt+100<=millis() && cfg.tiltEna) {
    if(misc.isApproxEqual(myIMU.readFloatAccelX(),myIMU.readFloatAccelY(),myIMU.readFloatAccelZ(),cfg.upX,cfg.upY,cfg.upZ,cfg.tiltTol) && !wasTilt) {
      Serial.println(F("[I] Tilt: UP!"));
      wasTilt = true;
      oldanim = currentAnim;
      tiltChange = millis();
      loadAnim(cfg.aUp,"");
    } else if (misc.isApproxEqual(myIMU.readFloatAccelX(),myIMU.readFloatAccelY(),myIMU.readFloatAccelZ(),cfg.tiltX,cfg.tiltY,cfg.tiltZ,cfg.tiltTol) && !wasTilt) {
      Serial.println(F("[I] Tilt: Side!"));
      wasTilt = true;
      oldanim = currentAnim;
      tiltChange = millis();
      loadAnim(cfg.aTilt,"");
    } else if ((tiltChange+revertTilt<millis() || misc.isApproxEqual(myIMU.readFloatAccelX(),myIMU.readFloatAccelY(),myIMU.readFloatAccelZ(),cfg.neutralX,cfg.neutralY,cfg.neutralZ,cfg.tiltTol)) && wasTilt) {
      Serial.println(F("[I] Tilt: Neutral!"));
      wasTilt = false;
      loadAnim(oldanim,"");
    }
    lastMillsTilt = millis();
  } else if (!tiltInitDone && cfg.tiltEna) {
    if(myIMU.begin()) {
      Serial.println(F("[E] An Error has occurred while connecting to LSM!"));
      cfg.tiltEna = false;
    } else {
      tiltInitDone = true;
    }
  }

  //looptime = micros();
  //--------------------------------//SPEECH Detection
  if(cfg.speechEna) { //~~8.8ms qwq~~ nuuh 1.2 with 32samples
    int nvol = 0, micline = 0, rawInput = 0;
    for (int i = 0; i<32; i++){
      adc_oneshot_read(adc_handle, MICpin, &rawInput);
      micline = abs(rawInput - 512);
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
        oled.speak(true);
      }
      Serial.println(F("[I] Speak"));
    }
    if(lastSpeak+500<millis() && speak) {
      speak = false;
      speechFirst = true;
      if(cfg.oledEna && oledInitDone) {
        oled.speak(false);
      }
      Serial.println(F("[I] unSpeak"));
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
      setAllVisor(visorLedsNEW,0,currentVisorFrame-1);
    }
    lastMillsSpeechAnim = millis();
  }
  //--------------------------------//SPEECH Reset frames
  if(!speechResetDone && !speak && lastMillsSpeechAnim+800<millis()) {
    if(visorNow->type == 0) { //custom
      setAllVisor(visorLedsNEW,0,currentVisorFrame-1);
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
      if(btnNum >= totalAnims) {
        btnNum = 0;
      } else {
        animToLoad = availAnims[btnNum];
      }
      Serial.println("Changing to "+availAnims[btnNum]+", amount of anims: "+String(totalAnims));
    }
  }

  //--------------------------------//BOOP Detection
  if(millis() > 200 && cfg.boopEna) {
    if(boopMode == "IR-KY") {
      digitalWrite(T_en, HIGH);
      delayMicroseconds(210);
      if(booping == false && !digitalRead(T_in)) {
        delayMicroseconds(395);
        if(!digitalRead(T_in)) {
          Serial.println(F("[I] IR BOOP"));
          booping = true;
          boopoldanim = currentAnim;
          loadAnim(cfg.aBoop,"");
          lastMillsBoop = millis();
        }
        digitalWrite(T_en, LOW);
      } else if(booping == true && lastMillsBoop+1000<millis() && digitalRead(T_in)) {
        digitalWrite(T_en, LOW);
        Serial.println(F("[I] IR unBOOP"));
        booping = false;
        if(!wasTilt) {
          loadAnim(boopoldanim,"");
        }
      }
    } else if (boopMode == "Capac") {
      if(booping == false && digitalRead(T_in)) {
        delayMicroseconds(395);
        if(digitalRead(T_in)) {
          Serial.println(F("[I] Touch BOOP"));
          booping = true;
          boopoldanim = currentAnim;
          loadAnim(cfg.aBoop,"");
          lastMillsBoop = millis();
        }
      } else if(booping == true && lastMillsBoop+1000<millis() && !digitalRead(T_in)) {
        Serial.println(F("[I] Touch unBOOP"));
        booping = false;
        if(!wasTilt) {
          loadAnim(boopoldanim,"");
        }
      }
    } else if (boopMode == "IR-Dist") {
      //TBD
    }
  }

  //--------------------------------//OLED routine, ~~10ms qwq~~, 1-5ms.. eh better
  if(cfg.oledEna && oledInitDone && vaStatLast+1000<millis()) {
    //looptime = micros();
    if(INApresent) {
      oled.writeINA(ina219.getBusVoltage_V(),ina219.getCurrent_mA());
    }
    if(cfg.bleEna) {
      if(pServer->getConnectedCount() == 0) {
        remoteSign = !remoteSign;
        oled.remote(remoteSign);
      } else if (pServer->getConnectedCount() > 0 && remoteSign == false) {
        oled.remote(true);
        remoteSign = true;
      }
    } else if (!cfg.bleEna && remoteSign) {
      oled.remote(false);
      remoteSign = false;
    }
    vaStatLast = millis();
    //Serial.println(">OLED:"+String(micros()-looptime));
  }
  if (!oledInitDone && cfg.oledEna) {
    if(!oled.init(oledAddr,cfg.bOled,INApresent)) {
      Serial.println(F("[E] An Error has occurred while initializing SSD1306."));
      cfg.oledEna = false;
    } else {
      oledInitDone = true;
    }
  }

  if((FdisplayEar || FdisplayBlush || FdisplayVisor) ) {
    if(visorType == "WS2812") {
      if(FdisplayVisor) {
        if(visorNow->type == 0) {
          if(FADESTEPS == 0) {
            memcpy(visorLeds, visorLedsNEW, sizeof(CRGB) * visorLedsNum);
            ledController[0]->showLeds(cfg.bVisor);
            FdisplayVisor = false;
          }else if(fadeTime + (visorLedsNum*0.03) < millis()) {
            for (uint16_t i = 0; i < visorLedsNum; i++) {
              visorLeds[i] = blend(visorLeds[i], visorLedsNEW[i], (currFade * 255) / FADESTEPS);
            }
            ledController[0]->showLeds(cfg.bVisor); //visor
            currFade++;
            if(currFade > FADESTEPS) {
              memcpy(visorLeds, visorLedsNEW, sizeof(CRGB) * visorLedsNum);
              FdisplayVisor = false;
              currFade = 1;
            }
          }
          /*for (uint8_t step = 1; step <= FADESTEPS; step++) {
            for (uint16_t i = 0; i < visorLedsNum; i++) {
              visorLeds[i] = blend(visorLeds[i], visorLedsNEW[i], (step * 255) / FADESTEPS);
            }
            ledController[0]->showLeds(cfg.bVisor); //visor
            delay(21); 
          }
          memcpy(visorLeds, visorLedsNEW, sizeof(CRGB) * visorLedsNum);
          FdisplayVisor = false;*/
        } else {
          ledController[0]->showLeds(cfg.bVisor);
          FdisplayVisor = false;
        }
      }
    } else if (visorType == "MAX72XX") {
      if(cfg.bVisor > 15) { cfg.bVisor = 15;}
      mx.control(MD_MAX72XX::INTENSITY, cfg.bVisor);
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    }
    if(blushPresent && useRGBblush) {
      for(int i = 0; i < blushLedsNum; i++) {
        uint32_t temp = blushLeds[i].r;
        blushLeds[i].r = blushLeds[i].g;
        blushLeds[i].g = temp;
      }
    }
    if(earPresent && !blushPresent && FdisplayEar) { //RMT4 2 controllers fix
      for(int i = 0; i < earLedsNum; i++) {
        c2Leds[i] = earLeds[i];
      }
      ledController[1]->showLeds(cfg.bEar);
    } else if (blushPresent && !earPresent && FdisplayBlush) {
      for(int i = 0; i < blushLedsNum; i++) {
        c2Leds[i] = blushLeds[i];
      }
      ledController[1]->showLeds(cfg.bEar);
    } else if (blushPresent && earPresent && (FdisplayBlush || FdisplayEar)) { //ear-blush-ear
      for(int i = 0; i < (earLedsNum/2); i++) {
        c2Leds[i] = earLeds[i];
      }
      for(int i = (earLedsNum/2); i < (earLedsNum/2)+blushLedsNum; i++) {
        c2Leds[i] = blushLeds[i-(earLedsNum/2)];
      }
      for(int i = (earLedsNum/2)+blushLedsNum; i < earLedsNum+blushLedsNum; i++) {
        c2Leds[i] = earLeds[i-blushLedsNum];
      }
      ledController[1]->showLeds(cfg.bEar);
    }
    /*if(FdisplayEar && earPresent) {
      ledController[1]->showLeds(cfg.bEar); //ears
      FdisplayEar = false;
    }
    if(FdisplayBlush && blushPresent) {
      ledController[2]->showLeds(cfg.bBlush); //blush
      FdisplayBlush = false;
    }*/
  }

  //press boot button for 10sec to reset
  if(check0button+10000 < millis() && check0button+10500 > millis() && digitalRead(0) == LOW) {
    Serial.println(F("[I] Resetting to defaults"));
    cfg.setDefault();
    delay(20);
    ESP.restart();
  } else if (digitalRead(0) == HIGH && check0button+10000 > millis() && check0button < millis()) {
    check0button = 0;
  } else if (digitalRead(0) == LOW && check0button+10000 < millis() && check0button < millis()) {
    check0button = millis();
  }

  if(animToLoad != "") {
    loadAnim(animToLoad,"");
    animToLoad = "";
    wasTilt = false;
    booping = false;
  }
}
