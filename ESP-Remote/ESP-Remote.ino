#include <ezButton.h>
#define BTNTIME 1500
#define BUTTONS 7
ezButton buttonArray[BUTTONS] = {
  ezButton(18), //left
  ezButton(19), //right
  ezButton(2), //up
  ezButton(17), //down
  ezButton(16), //center
  ezButton(4), //up right
  ezButton(5)  //up left
};

unsigned long btnPressTime[BUTTONS];
int btnVal[BUTTONS];

#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#define SPIFFS LittleFS
#include <LittleFS.h>

#include "WiFi.h"
#include "ESPAsyncWebServer.h"+
#include <ArduinoJson.h>

AsyncWebServer server(80);

String wifiName = "ProtoRemote", wifiPass = "Proto123";

String btnAnims[BUTTONS];

//--------------------------------//BLE
#define CONFIG_BT_NIMBLE_MAX_CONNECTIONS 1
#define CONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED
#define CONFIG_BT_NIMBLE_ROLE_BROADCASTER_DISABLED
#include "NimBLEDevice.h"

static BLEUUID serviceUUID("FFE0");
static BLEUUID charUUID("FFE1");

NimBLERemoteCharacteristic* pRemoteCharacteristic;
NimBLEAdvertisedDevice* pAdvertisedDevice;
NimBLEClient* pClient;

bool doConnect = false;
bool connected = false;
bool buttonPressed = false;
uint32_t scanTime = 0; // Duration of the scan
String foundDevices = "", BLE = "ProtoESP";

// BLE Scan callback
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    // Add device name to found devices list
    if (advertisedDevice->getName().length() > 0) {
      if (foundDevices.length() > 0) {
        foundDevices += ",";
      }
      foundDevices += String(advertisedDevice->getName().c_str());
      Serial.println("Found "+String(advertisedDevice->getName().c_str())+", finding "+BLE);
    }
    if (advertisedDevice->getName() == std::string(BLE.c_str(), BLE.length())) {
      advertisedDevice->getScan()->stop();
      pAdvertisedDevice = advertisedDevice;
      doConnect = true;
    }
  }
};

// Perform a new scan and get a comma-separated list of found devices
void scanBLE() {
  // Clear the previous list of found devices
  foundDevices = "";

  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setActiveScan(true);
  pScan->start(5, nullptr, false);
}

// Connect to the BLE server
bool connectToServer() {
  pClient = NimBLEDevice::createClient();

  if (pClient->connect(pAdvertisedDevice)) {
    Serial.println("Connected to server.");
    pRemoteCharacteristic = pClient->getService(serviceUUID)->getCharacteristic(charUUID);

    if (pRemoteCharacteristic == nullptr) {
      Serial.println("Failed to find characteristic.");
      pClient->disconnect();
      return false;
    }

    connected = true;
    return true;
  } else {
    Serial.println("Failed to connect to server.");
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
    btnAnims[i] = doc[String(i+1)].as<String>();
  }
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
    doc[String(i+1)] = btnAnims[i];
  }
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
    btnAnims[i] = "default";
  }
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
      Serial.println("Sent ?");
      delay(100);
      String response = pRemoteCharacteristic->readValue();
      request->send(200, "text/plain", response);
      Serial.println("Received: " + response);
    }
    request->send(503);
  });

  server.on("/scanBLE", HTTP_GET, [](AsyncWebServerRequest *request){ //all available BLE -----will timeout watchdog
    scanBLE();
    request->send(200, "text/plain", "Started scan");
  });

  server.on("/getBLE", HTTP_GET, [](AsyncWebServerRequest *request){ //all available BLE -----will timeout watchdog
    request->send(200, "text/plain", foundDevices);
  });

  server.on("/saveconfig", HTTP_GET, [](AsyncWebServerRequest *request){ //saves config
    //wifi
    for (int i = 0; i < BUTTONS; i++) {
      if(request->hasParam(String(i+1)))
        btnAnims[i] = String(request->getParam(String(i+1))->value());
    }
    if(request->hasParam("BLE"))
      BLE = String(request->getParam("BLE")->value());
    if(request->hasParam("wifiName"))
      wifiName = String(request->getParam("wifiName")->value());
    if(request->hasParam("wifiPass"))
      wifiPass = String(request->getParam("wifiPass")->value());
    if(saveConfig()) {
      request->redirect("/saved.html?main");
    } else {
      request->send(200, "text/plain", "Saving config failed!");
    }
  });

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.onNotFound([](AsyncWebServerRequest *request){request->send(404, "text/plain", "Not found");});
  server.begin();
}

//--------------------------------//Setup
void setup() {
  Serial.begin(115200);

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
  scanBLE();

  Serial.println("[I] Free heap: "+String(ESP.getFreeHeap()));
}

void loop() {
  for (int i = 0; i < BUTTONS; i++) {
    buttonArray[i].loop();
  }

  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Successfully connected to server.");
    } else {
      Serial.println("Failed to connect. Retrying...");
      NimBLEDevice::getScan()->start(scanTime, false);
    }
    doConnect = false;
  }

  if (connected && !pClient->isConnected()) {
    Serial.println("Disconnected from server. Reconnecting...");
    connected = false;
    NimBLEDevice::getScan()->start(scanTime, false);
  }

  if (connected) {
    for (int i = 0; i < BUTTONS; i++) {
      if(buttonArray[i].isPressed() && connected) { //detect press
        btnPressTime[i] = millis();
      }
      if(buttonArray[i].isReleased() && connected) { //short press
        long pressDuration = millis() - btnPressTime[i];
        if(pressDuration < BTNTIME) {
          if (pRemoteCharacteristic->canWrite()) {
            pRemoteCharacteristic->writeValue(btnAnims[i]);
            Serial.print("Sent: ");
            Serial.println(btnAnims[i]);
          }
          btnPressTime[i] = 20000000;
        }
      }
    }
  }

  delay(50); // Prevent spamming the loop
}
