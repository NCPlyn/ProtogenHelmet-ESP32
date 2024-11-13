#include <ezButton.h>
#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#include <LittleFS.h>

#define BTNTIME 800
#define BUTTONS 7
ezButton buttonArray[BUTTONS] = { //1.. left to right, row by row
  ezButton(8), //up left
  ezButton(2), //up right
  ezButton(3), //up
  ezButton(7), //left
  ezButton(9), //center
  ezButton(1), //right
  ezButton(4)  //down
};
unsigned long btnPressTime[BUTTONS];
String btnAnims[3][BUTTONS];

#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <ElegantOTA.h>
#include <ArduinoJson.h>
AsyncWebServer server(80);

String wifiName = "ProtoRemote", wifiPass = "Proto123";

//--------------------------------//BLE
#define CONFIG_BT_NIMBLE_MAX_CONNECTIONS 1
#define CONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED
#define CONFIG_BT_NIMBLE_ROLE_BROADCASTER_DISABLED
#include "NimBLEDevice.h"

static BLEUUID serviceUUID("FFE0");
static BLEUUID charUUID("FFE1");

NimBLERemoteCharacteristic* pRemoteCharacteristic;
NimBLERemoteService* pRemoteService;
NimBLEAdvertisedDevice* pAdvertisedDevice;
NimBLEClient* pClient;

bool doConnect = false,connected = false,buttonPressed = false;
String foundDevices = "", BLE = "ProtoESP";
int connectTry = 0, functionBtn = -1, animSet = 0;
unsigned long check0button = 0;

// BLE Scan callback, get a comma-separated list of found devices and check for valid one to connect to
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    // Add device name to found devices list
    if (advertisedDevice->getName().length() > 0) {
      if (foundDevices.length() > 0) {
        foundDevices += ",";
      }
      foundDevices += String(advertisedDevice->getName().c_str());
      Serial.println("[I] BT: Found: "+String(advertisedDevice->getName().c_str())+", finding: "+BLE);
    }
    // If it's the one were finding, stop scan and set bool to connect
    if (advertisedDevice->getName() == std::string(BLE.c_str(), BLE.length())) {
      Serial.println("[I] BT: Found correct: "+BLE);
      //advertisedDevice->getScan()->stop();
      pAdvertisedDevice = advertisedDevice;
      doConnect = true;
    }
  }
};

// Perform a new scan and set callback for found device
void scanBLE(int time) {
  foundDevices = "";
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setActiveScan(true);
  pScan->start(time, nullptr, false);
}

// Connect to the BLE server and check for valid service/char
bool connectToServer() {
  pClient = NimBLEDevice::createClient();

  if (pClient->connect(pAdvertisedDevice)) {
    Serial.println("[I] BT: Connected to server to get services/chars.");

    pRemoteService = pClient->getService(serviceUUID);
    if(pRemoteService != nullptr) {
      Serial.println("[I] BT: Got valid service.");
      pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
      if(pRemoteCharacteristic != nullptr) {
        Serial.println("[I] BT: Got valid characteristic.");
        connected = true;
        return true;
      }
    }
    Serial.println("[I] BT: Failed to find service or characteristic.");
    pClient->disconnect();
    NimBLEDevice::deleteClient(pClient);
    return false;
  } else {
    Serial.println("[I] BT: Failed to connect to server.");
    NimBLEDevice::deleteClient(pClient);
    return false;
  }
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

  for (int i = 0; i < BUTTONS; i++) {
    btnAnims[0][i] = doc[String(i+1+10)].as<String>();
    btnAnims[1][i] = doc[String(i+1+20)].as<String>();
    btnAnims[2][i] = doc[String(i+1+30)].as<String>();
  }
  functionBtn = doc["functionBtn"].as<int>();
  wifiName = doc["wifiName"].as<String>();
  wifiPass = doc["wifiPass"].as<String>();
  BLE = doc["BLE"].as<String>();

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

  for (int i = 0; i < BUTTONS; i++) {
    doc[String(i+1+10)] = btnAnims[0][i];
    doc[String(i+1+20)] = btnAnims[1][i];
    doc[String(i+1+30)] = btnAnims[2][i];
  }
  doc["functionBtn"] = functionBtn;
  doc["wifiName"] = wifiName;
  doc["wifiPass"] = wifiPass;
  doc["BLE"] = BLE;

  if(serializeJson(doc, file) == 0) {
    Serial.println("[E] Failed to deserialize the config file");
    return false;
  }

  file.close();
  Serial.println("[I] Config file saved");

  return true;
}

void setDefault() {
  for (int i = 0; i < BUTTONS; i++) {
    btnAnims[0][i] = "default";
    btnAnims[1][i] = "default";
    btnAnims[2][i] = "default";
  }
  functionBtn = -1;
  wifiName = "ProtoRemote";
  wifiPass = "Proto123";
  BLE = "ProtoBLE";
}

//--------------------------------//WiFi server setup
void startWiFiWeb() {
  WiFi.softAP(wifiName, wifiPass);

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.on("/getanims", HTTP_GET, [](AsyncWebServerRequest *request){ //all availble anims
    String temp;
    if(connected && pRemoteCharacteristic->canWrite() && pRemoteCharacteristic->canRead()) {
      pRemoteCharacteristic->writeValue("?");
      Serial.println("[I] BT: Sent ?");
      delay(100);
      String response = pRemoteCharacteristic->readValue();
      Serial.println("[I] BT: Received: " + response);
      request->send(200, "text/plain", response);
    } else {
      request->send(503);
    }
  });

  server.on("/scanBLE", HTTP_GET, [](AsyncWebServerRequest *request){ //all available BLE
    scanBLE(5);
    request->send(200, "text/plain", "Started scan");
  });

  server.on("/getBLE", HTTP_GET, [](AsyncWebServerRequest *request){ //return found ble devices
    if(connected && pRemoteCharacteristic->canWrite() && pRemoteCharacteristic->canRead()) { //bc connected doesn't appear while scanning
      request->send(200, "text/plain", BLE+","+foundDevices);
    } else {
      request->send(200, "text/plain", foundDevices);
    }
  });

  server.on("/saveconfig", HTTP_GET, [](AsyncWebServerRequest *request){ //saves config
    //btns
    for (int i = 0; i < BUTTONS; i++) {
      if(request->hasParam(String(i+1+10)))
        if(String(request->getParam(String(i+1+10))->value()) != "") {
          btnAnims[0][i] = String(request->getParam(String(i+1+10))->value());
      }
      if(request->hasParam(String(i+1+20)))
        if(String(request->getParam(String(i+1+20))->value()) != "") {
          btnAnims[1][i] = String(request->getParam(String(i+1+20))->value());
      }
      if(request->hasParam(String(i+1+30)))
        if(String(request->getParam(String(i+1+30))->value()) != "") {
          btnAnims[2][i] = String(request->getParam(String(i+1+30))->value());
      }
    }
    if(request->hasParam("functionBtn")) {
      int temp = request->getParam("functionBtn")->value().toInt();
      if(temp < 8 && temp > 0) {
        functionBtn = temp-1;
      } else {
        functionBtn = -1;
      }
    }
    //wifi/ble
    if(request->hasParam("BLE"))
      BLE = String(request->getParam("BLE")->value());
    if(request->hasParam("wifiName"))
      wifiName = String(request->getParam("wifiName")->value());
    if(request->hasParam("wifiPass"))
      wifiPass = String(request->getParam("wifiPass")->value());
    if(saveConfig()) {
      connectTry = 0;
      request->redirect("/saved.html?main");
    } else {
      request->send(200, "text/plain", "Saving config failed!");
    }
  });

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.onNotFound([](AsyncWebServerRequest *request){request->send(404, "text/plain", "Not found");});
  ElegantOTA.begin(&server);
  server.begin();

  ElegantOTA.setAutoReboot(true);
}

//--------------------------------//Setup
void setup() {
  Serial.begin(115200);
  
  pinMode(0, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  if(!LittleFS.begin(true)) {
    Serial.println("[E] An Error has occurred while mounting LittleFS! Halting");
    while(1){};
  }

  if(!loadConfig()) {
    Serial.println("[E] An Error has occurred while loading config file! Loading defaults");
    setDefault();
  }

  for (int i = 0; i < BUTTONS; i++) {
    buttonArray[i].setDebounceTime(50);
    btnPressTime[i] = 20000000;
  }

  startWiFiWeb();

  NimBLEDevice::init("");
  scanBLE(0);

  Serial.println("[I] Free heap: "+String(ESP.getFreeHeap()));
}

//--------------------------------//Loop
void loop() {
  ElegantOTA.loop();
  for (int i = 0; i < BUTTONS; i++) {
    buttonArray[i].loop();
  }

  //connect/reconnect to BLE
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("[I] BT: Successfully connected to server.");
    } else {
      if(connectTry < 3) {
        Serial.println("[I] BT: Failed to connect. Retrying...");
        connectTry++;
        NimBLEDevice::getScan()->start(0,nullptr, false);
      } else {
        Serial.println("[I] BT: Tried to connect 3 times and failed all 3, restart ESP or change BLE Server!");
      }
    }
    doConnect = false;
  }
  if (connected && !pClient->isConnected()) {
    Serial.println("[I] BT: Disconnected from server. Reconnecting...");
    connected = false;
    NimBLEDevice::getScan()->start(0,nullptr, false);
  }

  //check & send button press
  if(connected) {
    for(int i = 0; i < BUTTONS; i++) {
      if(buttonArray[i].isPressed()) { //detect press
        btnPressTime[i] = millis();
      }
      if(buttonArray[i].isReleased()) { //short press
        long pressDuration = millis() - btnPressTime[i];
        if(pressDuration < BTNTIME) {
          if(i == functionBtn) {
            pRemoteCharacteristic->writeValue(";rgb");
            Serial.println("[I] BT: Sent: ;rgb");
          } else {
            if (pRemoteCharacteristic->canWrite()) {
              pRemoteCharacteristic->writeValue(btnAnims[animSet][i]);
              Serial.print("[I] BT: Sent: ");
              Serial.println(btnAnims[animSet][i]);
            }
          }
          //btnPressTime[i] = 20000000;
        } else if (pressDuration > BTNTIME && pressDuration < BTNTIME+5000) {
          if(animSet==2) {animSet=0;} else {animSet++;}
          Serial.println("[I] Animation set to: "+String(animSet));
          pRemoteCharacteristic->writeValue(";set"+String(animSet));
          Serial.println("[I] BT: Sent: ;set"+String(animSet));
        }
      }
    }
  }

  //status LED
  if(connected && digitalRead(LED_BUILTIN) == HIGH) {
    digitalWrite(LED_BUILTIN,LOW);
  } else if (!connected && digitalRead(LED_BUILTIN) == LOW) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  
  //press boot button for 10sec to reset
  if(check0button+10000 < millis() && check0button+10500 > millis() && digitalRead(0) == LOW) {
    Serial.println("[I] Resetting to defaults");
    setDefault();
    delay(20);
    ESP.restart();
  } else if (digitalRead(0) == HIGH && check0button+10000 > millis() && check0button < millis()) {
    check0button = 0;
  } else if (digitalRead(0) == LOW && check0button+10000 < millis() && check0button < millis()) {
    check0button = millis();
    Serial.println("[I] set def wait");
  }

  delay(20); // Prevent spamming the loop?
}
